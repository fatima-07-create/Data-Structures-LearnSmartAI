"""
engine_client.py - the Python bridge to the C++ engine.

How it works (simple version):
  1. We start LearnSmart.exe with the flag --engine. It keeps running in the background.
  2. We write ONE command per line to its input, e.g.  GET_STUDENT|1001
  3. We read ONE reply back. Replies start with OK| or ERROR|.
     List replies are followed by ROW|... lines and finish with END.

Every command is listed in docs/PROTOCOL.md.
"""
from __future__ import annotations

import os
import queue
import subprocess
import threading
from pathlib import Path
from typing import Any, Dict, List, Optional

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATA_FOLDER = Path(os.environ.get("LEARNSMART_DATA", str(PROJECT_ROOT / "data")))   # LEARNSMART_DATA is used by the tests

# These commands answer with several lines (OK line, ROW lines, END line).
LIST_COMMANDS = {
    "LIST_STUDENTS", "LIST_ATTEMPTS", "SEARCH_TOPIC", "LIST_TASKS",
    "INSPECT_HASH", "INSPECT_STACK", "INSPECT_QUEUE",
}

ANSWER_TIMEOUT_SECONDS = 10


class EngineError(Exception):
    """A friendly error message that the screens can show to the student."""


def find_engine_executable() -> Optional[Path]:
    """Looks for the compiled C++ program in the usual places."""
    from_env = os.environ.get("LEARNSMART_ENGINE")
    if from_env and Path(from_env).is_file():
        return Path(from_env)

    names = ["LearnSmart.exe", "LearnSmart"]
    folders = [
        PROJECT_ROOT / "bin" / "x64" / "Release",     # Visual Studio (Release)
        PROJECT_ROOT / "bin" / "x64" / "Debug",       # Visual Studio (Debug)
        PROJECT_ROOT / "build",                       # CMake / g++ builds
        PROJECT_ROOT / "build" / "Release",
        PROJECT_ROOT / "build" / "Debug",
        PROJECT_ROOT / "out" / "build" / "x64-Debug",    # Visual Studio "Open Folder" (CMake)
        PROJECT_ROOT / "out" / "build" / "x64-Release",
        PROJECT_ROOT,
    ]
    for folder in folders:
        for name in names:
            candidate = folder / name
            if candidate.is_file():
                return candidate
    return None


def _clean(value: Any) -> str:
    """Pipes and new lines would break the command format, so swap them for spaces."""
    text = str(value)
    for bad in ("|", "\n", "\r"):
        text = text.replace(bad, " ")
    return text.strip()


class Reply:
    """One parsed answer from the engine."""

    def __init__(self, ok: bool, fields: List[str], rows: List[List[str]], message: str):
        self.ok = ok
        self.fields = fields      # the values after OK|
        self.rows = rows          # one list per ROW| line
        self.message = message    # error text, or the OK text


class EngineClient:
    def __init__(self, executable: Path, data_folder: Path = DATA_FOLDER):
        self.executable = Path(executable)
        self.data_folder = Path(data_folder)
        self._process: Optional[subprocess.Popen] = None
        self._lines: "queue.Queue[Optional[str]]" = queue.Queue()
        self._lock = threading.Lock()

    # ------------------------------------------------------------ process

    def _start(self) -> None:
        self._lines = queue.Queue()
        self._process = subprocess.Popen(
            [str(self.executable), "--engine", "--data", str(self.data_folder)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            encoding="utf-8",
            errors="replace",
            bufsize=1,
        )
        # The reader gets ITS OWN queue, so an old reader can never write into a newer engine's queue.
        reader = threading.Thread(target=self._read_forever, args=(self._process, self._lines), daemon=True)
        reader.start()

    @staticmethod
    def _read_forever(process: subprocess.Popen, lines: "queue.Queue[Optional[str]]") -> None:
        """Runs in the background and puts every output line into a queue."""
        assert process.stdout is not None
        for line in process.stdout:
            lines.put(line.rstrip("\r\n"))
        lines.put(None)           # the engine closed its output
        process.stdout.close()

    def _is_running(self) -> bool:
        return self._process is not None and self._process.poll() is None

    def _read_line(self) -> str:
        try:
            line = self._lines.get(timeout=ANSWER_TIMEOUT_SECONDS)
        except queue.Empty:
            self.stop()
            raise EngineError("The engine did not answer in time. Please try again.")
        if line is None:
            self.stop()
            raise EngineError("The engine stopped unexpectedly. Please try again.")
        return line

    def stop(self) -> None:
        process = self._process
        self._process = None
        if process is None:
            return
        if process.poll() is None:
            process.kill()
        process.wait()
        if process.stdin is not None:
            try:
                process.stdin.close()
            except OSError:
                pass

    def close(self) -> None:
        """Politely asks the engine to save and exit."""
        with self._lock:
            if self._is_running():
                try:
                    assert self._process is not None and self._process.stdin is not None
                    self._process.stdin.write("EXIT\n")
                    self._process.stdin.flush()
                    self._process.wait(timeout=3)
                except Exception:
                    pass
            self.stop()

    # ------------------------------------------------------------ commands

    def send(self, command: str, *arguments: Any) -> Reply:
        """Sends one command and returns the parsed reply (never raises for ERROR| replies)."""
        line = "|".join([command] + [_clean(a) for a in arguments])
        with self._lock:
            if not self._is_running():
                self._start()
            assert self._process is not None and self._process.stdin is not None
            try:
                self._process.stdin.write(line + "\n")
                self._process.stdin.flush()
            except OSError:
                self.stop()
                raise EngineError("Could not talk to the engine. Please try again.")

            first = self._read_line()
            if first.startswith("ERROR|"):
                return Reply(False, [], [], first[len("ERROR|"):])
            if not first.startswith("OK|"):
                raise EngineError("The engine sent an unexpected reply: " + first)

            fields = first.split("|")[1:]
            rows: List[List[str]] = []
            if command in LIST_COMMANDS:
                while True:
                    next_line = self._read_line()
                    if next_line == "END":
                        break
                    if next_line.startswith("ROW|"):
                        rows.append(next_line.split("|")[1:])
            return Reply(True, fields, rows, "|".join(fields))

    def call(self, command: str, *arguments: Any) -> Reply:
        """Like send(), but raises EngineError (with the engine's message) when the engine says ERROR."""
        reply = self.send(command, *arguments)
        if not reply.ok:
            raise EngineError(reply.message)
        return reply

    # ------------------------------------------------- friendly helpers

    def ping(self) -> bool:
        return self.send("PING").ok

    def list_students(self) -> List[Dict[str, Any]]:
        reply = self.call("LIST_STUDENTS")
        students = [
            {"id": int(r[0]), "name": r[1], "email": r[2], "attempts": int(r[3]), "tasks": int(r[4])}
            for r in reply.rows
        ]
        return sorted(students, key=lambda s: s["id"])

    def get_student(self, student_id: int) -> Optional[Dict[str, Any]]:
        """Returns the student's summary, or None if there is no such student."""
        reply = self.send("GET_STUDENT", student_id)
        if not reply.ok:
            return None
        f = reply.fields
        return {
            "id": int(f[0]), "name": f[1], "email": f[2],
            "attempts": int(f[3]), "pending_tasks": int(f[4]),
            "undo_size": int(f[5]), "average": float(f[6]),
        }

    def register_student(self, student_id: int, name: str, email: str) -> None:
        self.call("ADD_STUDENT", student_id, name, email)

    def remove_student(self, student_id: int) -> None:
        self.call("REMOVE_STUDENT", student_id)

    def add_attempt(self, student_id: int, topic_id: int, score: int, total: int, date: str) -> int:
        return int(self.call("ADD_ATTEMPT", student_id, topic_id, score, total, date).fields[0])

    @staticmethod
    def _attempt_rows(reply: Reply) -> List[Dict[str, Any]]:
        attempts = []
        for r in reply.rows:
            score, total = int(r[2]), int(r[3])
            attempts.append({
                "attempt_id": int(r[0]), "topic_id": int(r[1]),
                "score": score, "total": total, "date": r[4],
                "percent": (score * 100.0 / total) if total else 0.0,
            })
        return attempts

    def list_attempts(self, student_id: int) -> List[Dict[str, Any]]:
        return self._attempt_rows(self.call("LIST_ATTEMPTS", student_id))

    def search_topic(self, student_id: int, topic_id: int) -> List[Dict[str, Any]]:
        return self._attempt_rows(self.call("SEARCH_TOPIC", student_id, topic_id))

    def delete_attempt(self, student_id: int, attempt_id: int) -> None:
        self.call("DELETE_ATTEMPT", student_id, attempt_id)

    def add_task(self, student_id: int, topic_id: int, priority: int) -> int:
        return int(self.call("ADD_TASK", student_id, topic_id, priority).fields[0])

    def list_tasks(self, student_id: int) -> List[Dict[str, Any]]:
        reply = self.call("LIST_TASKS", student_id)
        return [
            {"task_id": int(r[0]), "topic_id": int(r[1]), "status": r[2], "priority": int(r[3])}
            for r in reply.rows
        ]

    def complete_task(self, student_id: int) -> Dict[str, int]:
        f = self.call("COMPLETE_TASK", student_id).fields
        return {"task_id": int(f[0]), "topic_id": int(f[1])}

    def undo(self, student_id: int) -> str:
        return self.call("UNDO", student_id).fields[0]

    def save(self) -> None:
        self.call("SAVE")

    # -------------------------------------------------------- inspector

    def hash_index(self, any_id: int) -> Dict[str, int]:
        f = self.call("HASH_INDEX", any_id).fields
        return {"bucket": int(f[0]), "bucket_count": int(f[1])}

    def inspect_hash(self) -> Dict[str, Any]:
        reply = self.call("INSPECT_HASH")
        f = reply.fields
        nodes = [
            {"bucket": int(r[0]), "position": int(r[1]), "id": int(r[2]), "name": r[3]}
            for r in reply.rows
        ]
        return {"bucket_count": int(f[0]), "student_count": int(f[1]), "collisions": int(f[2]), "nodes": nodes}

    def inspect_stack(self, student_id: int) -> Dict[str, Any]:
        reply = self.call("INSPECT_STACK", student_id)
        items = [
            {"position": int(r[0]), "type": r[1], "task_id": int(r[2]), "topic_id": int(r[3])}
            for r in reply.rows
        ]
        return {"size": int(reply.fields[0]), "capacity": int(reply.fields[1]), "items": items}   # top first

    def inspect_queue(self, student_id: int) -> Dict[str, Any]:
        reply = self.call("INSPECT_QUEUE", student_id)
        f = reply.fields
        slots = [
            {"slot": int(r[0]), "used": r[1] == "1", "task_id": int(r[2]),
             "topic_id": int(r[3]), "priority": int(r[4])}
            for r in reply.rows
        ]
        return {"front": int(f[0]), "rear": int(f[1]), "count": int(f[2]), "capacity": int(f[3]), "slots": slots}

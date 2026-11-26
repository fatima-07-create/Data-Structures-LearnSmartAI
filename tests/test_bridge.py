"""
Tests for the Python bridge and the C++ engine together.
Run from the project folder:   python -m unittest tests.test_bridge -v
Build the engine first (see README).
"""
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from streamlit_app.engine_client import EngineClient, EngineError, find_engine_executable   # noqa: E402


@unittest.skipIf(find_engine_executable() is None, "C++ engine is not built yet")
class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.folder = Path(tempfile.mkdtemp())
        shutil.copytree(ROOT / "data", self.folder / "data")           # a fresh copy of the demo data
        self.client = EngineClient(find_engine_executable(), self.folder / "data")

    def tearDown(self):
        self.client.close()
        shutil.rmtree(self.folder, ignore_errors=True)

    def test_ping_and_students(self):
        self.assertTrue(self.client.ping())
        students = self.client.list_students()
        self.assertEqual([s["id"] for s in students], [1001, 1002, 1003, 1012])

    def test_student_summary(self):
        student = self.client.get_student(1001)
        self.assertEqual(student["name"], "Ayesha Khan")
        self.assertEqual(student["attempts"], 4)
        self.assertEqual(student["average"], 45.0)
        self.assertIsNone(self.client.get_student(9999))

    def test_register_and_errors(self):
        self.client.register_student(2001, "New | Student", "n@x.com")      # the pipe is cleaned
        self.assertNotIn("|", self.client.get_student(2001)["name"])
        with self.assertRaises(EngineError) as caught:
            self.client.register_student(2001, "Again", "")
        self.assertIn("already exists", str(caught.exception))

    def test_quiz_history(self):
        attempt_id = self.client.add_attempt(1002, 3, 8, 10, "2026-11-01")
        attempts = self.client.list_attempts(1002)
        self.assertEqual(attempts[-1]["attempt_id"], attempt_id)
        self.assertEqual(len(self.client.search_topic(1002, 3)), 3)
        self.client.delete_attempt(1002, attempt_id)
        self.assertEqual(len(self.client.list_attempts(1002)), 3)
        with self.assertRaises(EngineError):
            self.client.add_attempt(1002, 3, 11, 10, "2026-11-01")           # score above total
        with self.assertRaises(EngineError):
            self.client.add_attempt(1002, 3, 5, 10, "01-11-2026")            # bad date

    def test_revision_planner_and_undo(self):
        task_id = self.client.add_task(1003, 5, 60)
        self.client.add_task(1003, 6, 40)
        self.assertEqual([t["topic_id"] for t in self.client.list_tasks(1003)], [5, 6])
        self.assertEqual(self.client.complete_task(1003), {"task_id": task_id, "topic_id": 5})   # FIFO
        self.assertIn("Completed task", self.client.undo(1003))                                  # LIFO
        self.assertEqual([t["topic_id"] for t in self.client.list_tasks(1003)], [5, 6])
        with self.assertRaises(EngineError):
            self.client.add_task(1003, 5, 10)                                # same topic twice

    def test_inspector_data(self):
        info = self.client.inspect_hash()
        self.assertEqual((info["bucket_count"], info["student_count"], info["collisions"]), (11, 4, 1))
        self.assertEqual(self.client.hash_index(1012), {"bucket": 0, "bucket_count": 11})
        for topic in range(1, 11):
            self.client.add_task(1003, topic, 50)
        for _ in range(3):
            self.client.complete_task(1003)
        for topic in range(11, 14):
            self.client.add_task(1003, topic, 50)                            # these wrap to slots 0 1 2
        queue = self.client.inspect_queue(1003)
        self.assertEqual((queue["front"], queue["rear"], queue["count"]), (3, 2, 10))
        self.assertTrue(all(slot["used"] for slot in queue["slots"]))
        stack = self.client.inspect_stack(1003)
        self.assertEqual(stack["items"][0]["type"], "AddTask")                # newest action on top

    def test_data_survives_restart(self):
        self.client.register_student(3001, "Saved Person", "")
        self.client.close()
        again = EngineClient(find_engine_executable(), self.folder / "data")
        try:
            self.assertEqual(again.get_student(3001)["name"], "Saved Person")
        finally:
            again.close()

    def test_engine_restarts_after_crash(self):
        self.client.ping()
        self.client.stop()                                                   # simulate a crash
        self.assertTrue(self.client.ping())


if __name__ == "__main__":
    unittest.main()

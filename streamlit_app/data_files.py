"""
data_files.py - reads the read-only content files (topics, questions, notes, resources).
These files are only for DISPLAY. Student data always goes through the C++ engine.
"""
from __future__ import annotations

import csv
from pathlib import Path
from typing import Any, Dict, List, Optional

from .engine_client import DATA_FOLDER


def _read_rows(file_name: str) -> List[Dict[str, str]]:
    path = Path(DATA_FOLDER) / file_name
    if not path.is_file():
        return []
    with open(path, newline="", encoding="utf-8-sig") as file:
        return list(csv.DictReader(file))


def load_topics() -> List[Dict[str, Any]]:
    topics = []
    for row in _read_rows("topics.csv"):
        try:
            topics.append({
                "topic_id": int(row["topic_id"]),
                "subject": row.get("subject", "General"),
                "name": row["topic_name"],
                "difficulty": row.get("difficulty", ""),
                "description": row.get("short_description", ""),
            })
        except (KeyError, ValueError):
            continue          # skip a broken row instead of crashing
    return topics


def topic_name(topic_id: int) -> str:
    for topic in load_topics():
        if topic["topic_id"] == topic_id:
            return topic["name"]
    return "Topic " + str(topic_id)


def topic_label(topic_id: int) -> str:
    """Example: 'Data Structures - Stack'."""
    for topic in load_topics():
        if topic["topic_id"] == topic_id:
            return topic["subject"] + " - " + topic["name"]
    return "Topic " + str(topic_id)


def subjects() -> List[str]:
    seen: List[str] = []
    for topic in load_topics():
        if topic["subject"] not in seen:
            seen.append(topic["subject"])
    return seen


def topics_in_subject(subject: str) -> List[Dict[str, Any]]:
    return [t for t in load_topics() if t["subject"] == subject]


def load_questions(topic_id: int) -> List[Dict[str, Any]]:
    questions = []
    for row in _read_rows("questions.csv"):
        try:
            if int(row["topic_id"]) != topic_id:
                continue
            options = row["options"].split(";")
            correct = "ABCD".index(row["correct_answer"].strip().upper())
            if correct >= len(options):
                continue
            questions.append({
                "id": int(row["question_id"]),
                "question": row["question"],
                "options": options,
                "correct_index": correct,
                "difficulty": row.get("difficulty", ""),
            })
        except (KeyError, ValueError):
            continue
    return questions


def load_notes(topic_id: int) -> Optional[Dict[str, Any]]:
    for row in _read_rows("tutor_notes.csv"):
        try:
            if int(row["topic_id"]) == topic_id:
                return {
                    "summary": row["summary"],
                    "key_points": [p.strip() for p in row["key_points"].split(";") if p.strip()],
                    "worked_example": row["worked_example"],
                    "common_mistakes": [p.strip() for p in row["common_mistakes"].split(";") if p.strip()],
                    "practice_questions": [p.strip() for p in row["practice_questions"].split(";") if p.strip()],
                }
        except (KeyError, ValueError):
            continue
    return None


def load_resources(topic_id: int) -> List[Dict[str, str]]:
    resources = []
    for row in _read_rows("learning_resources.csv"):
        try:
            if int(row["topic_id"]) == topic_id:
                resources.append({"title": row["resource_title"], "type": row["resource_type"], "url": row["resource_url"]})
        except (KeyError, ValueError):
            continue
    return resources


def topic_choices() -> List[tuple]:
    """List of (label, topic_id) for select boxes, in topic ID order."""
    return [(t["subject"] + " - " + t["name"], t["topic_id"]) for t in load_topics()]

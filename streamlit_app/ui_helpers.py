"""
ui_helpers.py - small helpers shared by all screens:
performance bands, colours, badges, flash messages and the topic summary used by the dashboard.
"""
from __future__ import annotations

import html
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import streamlit as st

from .engine_client import EngineError

STATUS_COLORS = {
    "Strong": "#16A34A",
    "Good": "#0D9488",
    "Needs Improvement": "#D97706",
    "Weak": "#DC2626",
}


# ------------------------------------------------------------------ styling

def load_css() -> None:
    css_file = Path(__file__).parent / "theme" / "style.css"
    if css_file.is_file():
        st.markdown("<style>" + css_file.read_text(encoding="utf-8") + "</style>", unsafe_allow_html=True)


def card(inner_html: str) -> None:
    st.markdown('<div class="ls-card">' + inner_html + "</div>", unsafe_allow_html=True)


def badge(label: str) -> str:
    """A coloured pill with a TEXT label (never colour alone)."""
    color = STATUS_COLORS.get(label, "#64748B")
    return '<span class="ls-badge" style="background:' + color + '">' + html.escape(label) + "</span>"


def page_title(title: str, subtitle: str = "") -> None:
    st.markdown("<h1 class='ls-title'>" + html.escape(title) + "</h1>", unsafe_allow_html=True)
    if subtitle:
        st.caption(subtitle)


def empty_state(text: str) -> None:
    st.info(text)


# ----------------------------------------------------------- flash messages
# A flash message survives st.rerun(), so the student sees "Saved!" after a button click.

def flash(kind: str, text: str) -> None:
    st.session_state["flash"] = (kind, text)


def show_flash() -> None:
    message = st.session_state.pop("flash", None)
    if not message:
        return
    kind, text = message
    if kind == "success":
        st.success(text)
    elif kind == "error":
        st.error(text)
    else:
        st.info(text)


# ------------------------------------------------------------ performance

def band_for(average: float) -> str:
    if average >= 80:
        return "Strong"
    if average >= 60:
        return "Good"
    if average >= 40:
        return "Needs Improvement"
    return "Weak"


def trend_for(percentages: List[float]) -> str:
    """Compares the latest attempt with the one before it (blueprint section 4.2)."""
    if len(percentages) < 2:
        return "Not enough attempts to compare"
    latest, previous = percentages[-1], percentages[-2]
    if latest > previous:
        return "Improving"
    if latest < previous:
        return "Declining"
    return "Stable"


def topic_performance(attempts: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    """
    One summary row per topic.
    average = (total marks obtained / total marks) x 100, as in blueprint section 4.1.
    NOTE: Member B's C++ engine will own the official version of this. This is a display helper.
    """
    by_topic: Dict[int, List[Dict[str, Any]]] = {}
    for attempt in attempts:                      # attempts arrive oldest first
        by_topic.setdefault(attempt["topic_id"], []).append(attempt)

    summary = []
    for topic_id, items in by_topic.items():
        obtained = sum(a["score"] for a in items)
        total = sum(a["total"] for a in items)
        average = (obtained * 100.0 / total) if total else 0.0
        summary.append({
            "topic_id": topic_id,
            "attempts": len(items),
            "average": average,
            "band": band_for(average),
            "latest": items[-1]["percent"],
            "trend": trend_for([a["percent"] for a in items]),
        })
    return sorted(summary, key=lambda s: (s["average"], s["topic_id"]))   # weakest first


def basic_next_topic(summary: List[Dict[str, Any]]) -> Optional[Tuple[Dict[str, Any], str]]:
    """
    A basic suggestion for the dashboard: the lowest-average topic that is not Strong.
    It does NOT check prerequisites yet - that arrives with Member B's recommendation engine.
    """
    for entry in summary:
        if entry["band"] != "Strong":
            reason = ("Your average is " + str(round(entry["average"])) + "% and your latest score is "
                      + str(round(entry["latest"])) + "%.")
            return entry, reason
    return None


# ------------------------------------------------------------- small tools

def run_action(action, success_text) -> None:
    """
    Runs a button action, remembers a success or error message, and reloads the page.
    success_text can be a plain string or a function that receives the action's result.
    """
    try:
        result = action()
    except EngineError as error:
        flash("error", str(error))
    else:
        flash("success", success_text(result) if callable(success_text) else success_text)
    st.rerun()


def html_table(headers: List[str], rows: List[List[str]]) -> str:
    """Builds a simple table. Cells must already be safe HTML (escape text with html.escape)."""
    head = "".join("<th style='text-align:left;padding:6px 10px;color:#64748B'>" + h + "</th>" for h in headers)
    body = ""
    for row in rows:
        body += "<tr>" + "".join(
            "<td style='padding:6px 10px;border-top:1px solid #E2E8F0'>" + cell + "</td>" for cell in row
        ) + "</tr>"
    return "<table style='width:100%;border-collapse:collapse'><tr>" + head + "</tr>" + body + "</table>"


def priority_for(summary: List[Dict[str, Any]], topic_id: int) -> int:
    """Priority = 100 - topic average (blueprint section 4.4). Topics with no attempts get 50."""
    for entry in summary:
        if entry["topic_id"] == topic_id:
            return max(0, min(100, round(100 - entry["average"])))
    return 50

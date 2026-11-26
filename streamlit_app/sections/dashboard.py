"""Dashboard: metric cards, topic performance, quiz history and the revision planner."""
from __future__ import annotations

import datetime
import html

import pandas as pd
import streamlit as st

from streamlit_app import data_files as data
from streamlit_app import ui_helpers as ui


def render(client, student) -> None:
    student_id = student["id"]
    attempts = client.list_attempts(student_id)
    tasks = client.list_tasks(student_id)
    summary = ui.topic_performance(attempts)

    ui.page_title("Dashboard", "Welcome back, " + student["name"] + ".")

    # ---- metric cards
    weak_count = len([s for s in summary if s["band"] == "Weak"])
    average_text = (str(round(student["average"])) + "%") if attempts else "-"
    columns = st.columns(4)
    columns[0].metric("Quizzes taken", len(attempts))
    columns[1].metric("Overall average", average_text)
    columns[2].metric("Weak topics", weak_count)
    columns[3].metric("Pending revision tasks", len(tasks))

    if not attempts:
        ui.empty_state("Take your first quiz to see your learning progress.")
    else:
        _next_topic_card(client, student_id, summary, tasks)
        _performance_chart(summary)

    tab_topics, tab_history, tab_revision = st.tabs(["Topic performance", "Quiz history", "Revision planner"])
    with tab_topics:
        _topic_table(summary)
    with tab_history:
        _quiz_history(client, student_id, attempts)
    with tab_revision:
        _revision_planner(client, student, tasks, summary)


# ------------------------------------------------------------------ pieces

def _next_topic_card(client, student_id, summary, tasks) -> None:
    st.subheader("Next recommended topic")
    suggestion = ui.basic_next_topic(summary)
    if suggestion is None:
        ui.card("<b>Great work!</b> Every topic you attempted is Strong. Try a new topic from the Quiz section.")
        return

    entry, reason = suggestion
    ui.card(
        "<b>" + html.escape(data.topic_label(entry["topic_id"])) + "</b> &nbsp; " + ui.badge(entry["band"])
        + "<div style='margin-top:6px'>" + html.escape(reason) + "</div>"
        + "<div class='ls-muted'>Trend: " + html.escape(entry["trend"]) + "</div>"
    )
    st.caption("Basic suggestion. The Learning Recommendations section explains the reason in more detail.")
    already_queued = entry["topic_id"] in {t["topic_id"] for t in tasks}
    if already_queued:
        st.caption("This topic is already in your revision queue.")
    if st.button("Add to revision queue", type="primary", key="dash_add_next", disabled=already_queued):
        priority = ui.priority_for(summary, entry["topic_id"])
        ui.run_action(
            lambda: client.add_task(student_id, entry["topic_id"], priority),
            "Added " + data.topic_name(entry["topic_id"]) + " to your revision queue.",
        )


def _performance_chart(summary) -> None:
    st.subheader("Topic performance")
    frame = pd.DataFrame(
        {"Average score (%)": [round(s["average"], 1) for s in summary]},
        index=[data.topic_name(s["topic_id"]) for s in summary],
    )
    st.bar_chart(frame, color="#0F766E")


def _topic_table(summary) -> None:
    if not summary:
        ui.empty_state("No topic results yet. Take a quiz to see your strong and weak topics.")
        return
    rows = []
    for s in summary:
        rows.append([
            html.escape(data.topic_label(s["topic_id"])),
            str(round(s["average"])) + "%",
            ui.badge(s["band"]),
            html.escape(s["trend"]),
            str(s["attempts"]),
        ])
    ui.card(ui.html_table(["Topic", "Average", "Level", "Progress", "Attempts"], rows))


def _quiz_history(client, student_id, attempts) -> None:
    topic_ids = sorted({a["topic_id"] for a in attempts})
    options = ["All topics"] + [data.topic_label(t) for t in topic_ids]
    chosen = st.selectbox("Show attempts for", options, key="history_filter")

    if chosen == "All topics":
        shown = attempts
    else:
        shown = client.search_topic(student_id, topic_ids[options.index(chosen) - 1])   # linked list search in C++

    if not shown:
        ui.empty_state("No quiz attempts to show yet.")
    else:
        rows = []
        for a in reversed(shown):                         # newest first
            rows.append([
                "#" + str(a["attempt_id"]),
                html.escape(data.topic_label(a["topic_id"])),
                str(a["score"]) + "/" + str(a["total"]),
                str(round(a["percent"])) + "%",
                ui.badge(ui.band_for(a["percent"])),
                html.escape(a["date"]),
            ])
        ui.card(ui.html_table(["Attempt", "Topic", "Score", "Percent", "Level", "Date"], rows))

        pick = st.selectbox(
            "Delete an attempt",
            [a["attempt_id"] for a in shown],
            format_func=lambda i: "Attempt #" + str(i),
            key="history_delete_pick",
        )
        if st.button("Delete selected attempt", key="history_delete_button"):
            ui.run_action(lambda: client.delete_attempt(student_id, pick), "Attempt #" + str(pick) + " deleted.")

    with st.expander("Record an attempt manually"):
        choices = data.topic_choices()
        with st.form("manual_attempt_form"):
            label = st.selectbox("Topic", [c[0] for c in choices])
            score = st.number_input("Score", min_value=0, value=10, step=1)
            total = st.number_input("Total marks", min_value=1, value=15, step=1)
            date = st.text_input("Date (YYYY-MM-DD)", value=datetime.date.today().isoformat())
            submitted = st.form_submit_button("Save attempt", type="primary")
        if submitted:
            topic_id = dict(choices)[label]
            ui.run_action(
                lambda: client.add_attempt(student_id, topic_id, int(score), int(total), date),
                lambda attempt_id: "Saved as attempt #" + str(attempt_id) + ".",
            )


def _revision_planner(client, student, tasks, summary) -> None:
    student_id = student["id"]
    st.write("Tasks are done in the order they were added (queue: first in, first out). "
             "Undo reverses your latest action (stack: last in, first out).")

    if not tasks:
        ui.empty_state("Your revision queue is empty. Add a topic below.")
    else:
        rows = []
        for position, task in enumerate(tasks, start=1):
            rows.append([
                str(position) + (" (next)" if position == 1 else ""),
                html.escape(data.topic_label(task["topic_id"])),
                str(task["priority"]),
                html.escape(task["status"]),
            ])
        ui.card(ui.html_table(["Order", "Topic", "Priority", "Status"], rows))

    left, right = st.columns(2)
    with left:
        if st.button("Complete next task", disabled=not tasks, key="plan_complete"):
            ui.run_action(
                lambda: client.complete_task(student_id),
                lambda done: "Completed " + data.topic_name(done["topic_id"]) + ".",
            )
    with right:
        if st.button("Undo last action", disabled=student["undo_size"] == 0, key="plan_undo"):
            ui.run_action(lambda: client.undo(student_id), lambda message: message)

    choices = data.topic_choices()
    with st.form("plan_add_form"):
        label = st.selectbox("Topic to revise", [c[0] for c in choices])
        priority = st.slider("Priority (higher = more urgent)", 0, 100, 50)
        submitted = st.form_submit_button("Add to revision queue", type="primary")
    if submitted:
        topic_id = dict(choices)[label]
        ui.run_action(
            lambda: client.add_task(student_id, topic_id, priority),
            "Added " + data.topic_name(topic_id) + " to your revision queue.",
        )

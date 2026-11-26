"""
Learning Recommendations: the recommended topic with a visible reason, then notes, resources and practice.
The prerequisite-aware logic (Graph + Heap) belongs to Member B's C++ engine. Until those commands exist
this screen shows a basic suggestion from the student's own scores.
"""
from __future__ import annotations

import html

import streamlit as st

from streamlit_app import data_files as data
from streamlit_app import ui_helpers as ui


def render(client, student) -> None:
    student_id = student["id"]
    attempts = client.list_attempts(student_id)
    summary = ui.topic_performance(attempts)
    queued_topics = {t["topic_id"] for t in client.list_tasks(student_id)}

    ui.page_title("Learning Recommendations", "What to study next, and why.")

    suggestion = ui.basic_next_topic(summary)
    st.subheader("Recommended topic")
    if not attempts:
        ui.empty_state("Take your first quiz to see your learning progress and get a recommendation.")
    elif suggestion is None:
        ui.card("<b>Nothing urgent.</b> All attempted topics are Strong. Try a new topic in the Quiz section.")
    else:
        entry, reason = suggestion
        ui.card(
            "<b>" + html.escape(data.topic_label(entry["topic_id"])) + "</b> &nbsp; " + ui.badge(entry["band"])
            + "<div style='margin-top:6px'><b>Why:</b> " + html.escape(reason) + "</div>"
            + "<div class='ls-muted'>Progress: " + html.escape(entry["trend"]) + "</div>"
        )
    st.caption("Prerequisite checking (Graph) and priority ranking (Heap) are added by Member B's engine commands.")

    _study_material(client, student_id, summary, queued_topics, suggestion[0]["topic_id"] if suggestion else None)


def _study_material(client, student_id, summary, queued_topics, recommended_topic_id) -> None:
    st.subheader("Study notes and resources")

    subjects = data.subjects()
    if not subjects:
        st.error("No topics were found. Check that data/topics.csv exists.")
        return

    # Start on the recommended topic when there is one.
    default_subject, default_index = 0, 0
    if recommended_topic_id is not None:
        for s_index, subject in enumerate(subjects):
            for t_index, topic in enumerate(data.topics_in_subject(subject)):
                if topic["topic_id"] == recommended_topic_id:
                    default_subject, default_index = s_index, t_index

    subject = st.selectbox("Subject", subjects, index=default_subject, key="rec_subject")
    topics = data.topics_in_subject(subject)
    names = [t["name"] for t in topics]
    index = default_index if subjects[default_subject] == subject else 0
    chosen = st.selectbox("Sub-topic", names, index=index, key="rec_topic_" + subject)
    topic = topics[names.index(chosen)]

    notes = data.load_notes(topic["topic_id"])
    if notes is None:
        ui.empty_state("No notes are available for this topic yet.")
    else:
        ui.card("<b>" + html.escape(topic["name"]) + "</b><br>" + html.escape(notes["summary"]))
        st.markdown("**Key points**")
        for point in notes["key_points"]:
            st.markdown("- " + point)
        st.markdown("**Worked example**")
        st.write(notes["worked_example"])
        st.markdown("**Common mistakes**")
        for mistake in notes["common_mistakes"]:
            st.markdown("- " + mistake)
        st.markdown("**Practice questions**")
        for number, practice in enumerate(notes["practice_questions"], start=1):
            st.markdown(str(number) + ". " + practice)

    resources = data.load_resources(topic["topic_id"])
    if resources:
        st.markdown("**Learning resources**")
        for resource in resources:
            st.markdown("- [" + resource["title"] + "](" + resource["url"] + ")  (" + resource["type"] + ")")

    already_queued = topic["topic_id"] in queued_topics
    if already_queued:
        st.caption("This topic is already in your revision queue.")
    if st.button("Add to revision queue", type="primary", key="rec_add", disabled=already_queued):
        priority = ui.priority_for(summary, topic["topic_id"])
        ui.run_action(
            lambda: client.add_task(student_id, topic["topic_id"], priority),
            "Added " + topic["name"] + " to your revision queue.",
        )

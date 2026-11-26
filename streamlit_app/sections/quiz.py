"""
Quiz: pick a subject and a sub-topic, answer one question per screen, see the score, save the attempt.
The attempt is stored by the C++ engine (Linked List). Answer checking is done here for now;
Member B can move scoring into the C++ engine later without changing this screen much.
"""
from __future__ import annotations

import datetime
import html

import streamlit as st

from streamlit_app import data_files as data
from streamlit_app import ui_helpers as ui

LETTERS = "ABCD"


def _go_to(section: str) -> None:
    st.session_state["section"] = section


def _reset_quiz() -> None:
    state = st.session_state.pop("quiz", None)
    if state:
        for index in range(len(state["questions"])):
            st.session_state.pop("quiz_q_" + str(index), None)


def render(client, student) -> None:
    ui.page_title("Quiz", "Choose a subject and a sub-topic, then answer the questions one by one.")
    state = st.session_state.get("quiz")
    if state is None:
        _picker()
    elif state["finished"]:
        _result(state)
    else:
        _question(client, student, state)


# ------------------------------------------------------------------ picker

def _picker() -> None:
    subjects = data.subjects()
    if not subjects:
        st.error("No topics were found. Check that data/topics.csv exists.")
        return

    subject = st.selectbox("Subject", subjects, key="quiz_subject")
    topics = data.topics_in_subject(subject)
    names = [t["name"] + " (" + t["difficulty"] + ")" for t in topics]
    chosen = st.selectbox("Sub-topic", names, key="quiz_topic_" + subject)
    topic = topics[names.index(chosen)]

    questions = data.load_questions(topic["topic_id"])
    ui.card("<b>" + html.escape(topic["name"]) + "</b><div class='ls-muted'>"
            + html.escape(topic["description"]) + "<br>" + str(len(questions)) + " questions</div>")

    if st.button("Start Quiz", type="primary", disabled=not questions):
        st.session_state["quiz"] = {
            "topic_id": topic["topic_id"], "questions": questions,
            "index": 0, "answers": {}, "finished": False, "attempt_id": None,
        }
        st.rerun()


# ---------------------------------------------------------------- question

def _question(client, student, state) -> None:
    questions = state["questions"]
    index = state["index"]
    question = questions[index]
    total = len(questions)

    st.caption(data.topic_label(state["topic_id"]))
    st.progress((index + 1) / total, text="Question " + str(index + 1) + " of " + str(total))
    st.markdown("### " + question["question"])

    options = [LETTERS[i] + ".  " + text for i, text in enumerate(question["options"])]
    saved = state["answers"].get(index)
    choice = st.radio("Choose one answer", options, index=saved, key="quiz_q_" + str(index))
    state["answers"][index] = options.index(choice) if choice is not None else None

    left, middle, right = st.columns([1, 1, 3])
    with left:
        if st.button("Back", disabled=index == 0, key="quiz_back"):
            state["index"] = index - 1
            st.rerun()
    with middle:
        if index < total - 1:
            if st.button("Next", type="primary", key="quiz_next"):
                state["index"] = index + 1
                st.rerun()
        else:
            if st.button("Submit Quiz", type="primary", key="quiz_submit"):
                missing = [i + 1 for i in range(total) if state["answers"].get(i) is None]
                if missing:
                    st.warning("Please answer question " + str(missing[0]) + " first ("
                               + str(len(missing)) + " unanswered).")
                else:
                    _finish(client, student, state)
    with right:
        if st.button("Cancel quiz", key="quiz_cancel"):
            _reset_quiz()
            st.rerun()


def _finish(client, student, state) -> None:
    questions = state["questions"]
    score = sum(1 for i, q in enumerate(questions) if state["answers"].get(i) == q["correct_index"])
    today = datetime.date.today().isoformat()
    try:
        state["attempt_id"] = client.add_attempt(student["id"], state["topic_id"], score, len(questions), today)
    except Exception as error:                                   # the engine said no
        st.error("Your score could not be saved: " + str(error))
        return
    state["score"] = score
    state["finished"] = True
    st.rerun()


# ------------------------------------------------------------------ result

def _result(state) -> None:
    questions = state["questions"]
    score, total = state["score"], len(questions)
    percent = score * 100.0 / total
    band = ui.band_for(percent)

    st.success("Quiz submitted. Saved as attempt #" + str(state["attempt_id"]) + ".")
    columns = st.columns(3)
    columns[0].metric("Score", str(score) + " / " + str(total))
    columns[1].metric("Percentage", str(round(percent)) + "%")
    columns[2].markdown("**Level**<br>" + ui.badge(band), unsafe_allow_html=True)

    with st.expander("Review your answers"):
        for i, q in enumerate(questions):
            picked = state["answers"].get(i)
            right = q["correct_index"]
            mark = "Correct" if picked == right else "Wrong"
            st.markdown("**" + str(i + 1) + ". " + q["question"] + "**  (" + mark + ")")
            st.write("Your answer: " + (q["options"][picked] if picked is not None else "none"))
            if picked != right:
                st.write("Correct answer: " + q["options"][right])

    left, right = st.columns(2)
    left.button("Take another quiz", type="primary", on_click=_reset_quiz)
    right.button("Go to Dashboard", on_click=lambda: (_reset_quiz(), _go_to("Dashboard")))

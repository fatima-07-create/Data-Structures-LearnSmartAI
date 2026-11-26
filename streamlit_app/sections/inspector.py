"""
Data Structure Inspector.
Member A's four structures (Hash Table, Linked List, Stack, Circular Queue) are live here.
The other four tabs are reserved for Member B's structures.
"""
from __future__ import annotations

import datetime
import html

import streamlit as st

from streamlit_app import data_files as data
from streamlit_app import ui_helpers as ui

ARROW = "<span class='ls-arrow'>&rarr;</span>"


def node(text: str, hot: bool = False, empty: bool = False) -> str:
    css = "ls-node" + (" hot" if hot else "") + (" empty" if empty else "")
    return "<span class='" + css + "'>" + html.escape(text) + "</span>"


def render(client, student) -> None:
    ui.page_title("Data Structure Inspector", "See how each structure stores LearnSmart data and try its main operations.")
    tabs = st.tabs(["Hash Table", "Linked List", "Stack", "Circular Queue", "BST", "AVL Tree", "Heap", "Graph"])

    with tabs[0]:
        _hash_table(client)
    with tabs[1]:
        _linked_list(client, student)
    with tabs[2]:
        _stack(client, student)
    with tabs[3]:
        _queue(client, student)

    member_b = {
        4: ("Binary Search Tree", "Indexes quiz attempts by score and answers score-range queries."),
        5: ("AVL Tree", "Stores the topic catalogue in balanced order using LL, RR, LR and RL rotations."),
        6: ("Max Heap", "Picks the highest-priority topics for revision."),
        7: ("Graph", "Shows which topics are prerequisites of other topics (BFS and DFS)."),
    }
    for tab_index, (name, purpose) in member_b.items():
        with tabs[tab_index]:
            st.subheader(name)
            st.write(purpose)
            ui.empty_state("This structure is part of Member B's work and will appear here once it is added.")


# -------------------------------------------------------------- Hash Table

def _hash_table(client) -> None:
    st.subheader("Hash Table")
    st.write("Purpose: find a student by ID in almost one step. The bucket is chosen with "
             "**student ID % number of buckets**. Students that land in the same bucket are chained together (collision).")

    info = client.inspect_hash()
    columns = st.columns(3)
    columns[0].metric("Buckets", info["bucket_count"])
    columns[1].metric("Students", info["student_count"])
    columns[2].metric("Collisions", info["collisions"])

    chains = {}
    for entry in sorted(info["nodes"], key=lambda n: (n["bucket"], n["position"])):
        chains.setdefault(entry["bucket"], []).append(entry)

    lines = ""
    for bucket in range(info["bucket_count"]):
        chain = chains.get(bucket, [])
        row = "<div class='ls-row'><span class='ls-tag'>Bucket " + str(bucket) + "</span>"
        for entry in chain:
            row += ARROW + node(str(entry["id"]) + " " + entry["name"])
        row += ARROW + node("NULL", empty=True)
        if len(chain) > 1:
            row += "<span class='ls-warn'>collision: " + str(len(chain)) + " students share this bucket</span>"
        lines += row + "</div>"
    ui.card(lines)

    st.markdown("**Try it**")
    left, right = st.columns(2)
    with left:
        test_id = st.number_input("Where would this student ID go?", min_value=1, value=1012, step=1, key="hash_test_id")
        where = client.hash_index(int(test_id))
        in_bucket = len(chains.get(where["bucket"], []))
        st.write(str(int(test_id)) + " % " + str(where["bucket_count"]) + " = bucket " + str(where["bucket"]) + ".")
        if in_bucket:
            st.write("That bucket already holds " + str(in_bucket) + " student(s), so adding this ID causes a collision.")
        else:
            st.write("That bucket is empty, so there is no collision.")

        found = st.button("Find student by this ID", key="hash_find")
        if found:
            result = client.get_student(int(test_id))
            if result is None:
                st.error("No student with ID " + str(int(test_id)) + ".")
            else:
                st.success("Found " + result["name"] + " in bucket " + str(where["bucket"]) + ".")
    with right:
        with st.form("hash_register_form"):
            new_id = st.number_input("New student ID", min_value=1, value=1012, step=1)
            name = st.text_input("Name", value="")
            email = st.text_input("Email", value="")
            submitted = st.form_submit_button("Register student (insert)", type="primary")
        if submitted:
            ui.run_action(lambda: client.register_student(int(new_id), name, email),
                          "Student " + str(int(new_id)) + " registered.")

        remove_id = st.number_input("Student ID to remove", min_value=1, value=1012, step=1, key="hash_remove_id")
        if st.button("Remove student (delete)", key="hash_remove"):
            ui.run_action(lambda: client.remove_student(int(remove_id)),
                          "Student " + str(int(remove_id)) + " removed.")


# ------------------------------------------------------------- Linked List

def _linked_list(client, student) -> None:
    st.subheader("Singly Linked List")
    st.write("Purpose: keep each student's quiz attempts in the order they happened. "
             "New attempts are added at the tail.")

    students = client.list_students()
    ids = [s["id"] for s in students]
    names = {s["id"]: s["name"] for s in students}
    chosen = st.selectbox(
        "Student", ids, index=ids.index(student["id"]) if student["id"] in ids else 0,
        format_func=lambda i: str(i) + " - " + names[i], key="ll_student",
    )

    attempts = client.list_attempts(chosen)
    topic_ids = sorted({a["topic_id"] for a in attempts})
    highlight_choice = st.selectbox(
        "Search: highlight attempts for topic", ["(none)"] + [data.topic_label(t) for t in topic_ids], key="ll_search",
    )
    highlighted = set()
    if highlight_choice != "(none)":
        labels = [data.topic_label(t) for t in topic_ids]
        topic_id = topic_ids[labels.index(highlight_choice)]
        highlighted = {a["attempt_id"] for a in client.search_topic(chosen, topic_id)}    # C++ linked list search

    if not attempts:
        ui.empty_state("This student has no quiz attempts yet. Insert one below.")
    else:
        row = "<div class='ls-row'><span class='ls-tag'>HEAD</span>"
        for index, a in enumerate(attempts):
            label = "#" + str(a["attempt_id"]) + " | T" + str(a["topic_id"]) + " | " + str(round(a["percent"])) + "%"
            row += ARROW + node(label, hot=a["attempt_id"] in highlighted)
            if index == len(attempts) - 1:
                row += "<span class='ls-tag'>(TAIL)</span>"
        row += ARROW + node("NULL", empty=True) + "</div>"
        ui.card(row + "<div class='ls-muted'>Each box is a node: attempt number, topic ID (T) and score percent. "
                + str(len(attempts)) + " node(s).</div>")

    st.markdown("**Try it**")
    left, right = st.columns(2)
    with left:
        choices = data.topic_choices()
        with st.form("ll_insert_form"):
            label = st.selectbox("Topic", [c[0] for c in choices])
            score = st.number_input("Score", min_value=0, value=10, step=1)
            total = st.number_input("Total marks", min_value=1, value=15, step=1)
            date = st.text_input("Date (YYYY-MM-DD)", value=datetime.date.today().isoformat())
            submitted = st.form_submit_button("Insert at tail", type="primary")
        if submitted:
            topic_id = dict(choices)[label]
            ui.run_action(
                lambda: client.add_attempt(chosen, topic_id, int(score), int(total), date),
                lambda attempt_id: "Inserted attempt #" + str(attempt_id) + " at the tail.",
            )
    with right:
        if attempts:
            pick = st.selectbox("Delete attempt", [a["attempt_id"] for a in attempts],
                                format_func=lambda i: "Attempt #" + str(i), key="ll_delete_pick")
            if st.button("Delete node", key="ll_delete"):
                ui.run_action(lambda: client.delete_attempt(chosen, pick), "Deleted attempt #" + str(pick) + ".")


# --------------------------------------------------------------- Stack

def _task_form(client, student_id, key: str, button_text: str) -> None:
    choices = data.topic_choices()
    with st.form(key):
        label = st.selectbox("Topic to revise", [c[0] for c in choices])
        priority = st.slider("Priority", 0, 100, 50)
        submitted = st.form_submit_button(button_text, type="primary")
    if submitted:
        topic_id = dict(choices)[label]
        ui.run_action(lambda: client.add_task(student_id, topic_id, priority),
                      "Added " + data.topic_name(topic_id) + " to the revision queue.")


def _stack(client, student) -> None:
    student_id = student["id"]
    st.subheader("Stack (array)")
    st.write("Purpose: undo the most recent revision action. The last action pushed is the first one undone (LIFO).")

    info = client.inspect_stack(student_id)
    st.write("Size: " + str(info["size"]) + " of " + str(info["capacity"]) + " (array capacity)")

    if not info["items"]:
        ui.empty_state("The undo stack is empty. Add or complete a revision task to push an undo record.")
    else:
        rows = ""
        for item in info["items"]:
            text = ("Added" if item["type"] == "AddTask" else "Completed") + " task #" + str(item["task_id"]) \
                   + " (" + data.topic_name(item["topic_id"]) + ")"
            tag = "<span class='ls-tag'>TOP</span>" if item["position"] == 0 else ""
            rows += "<div class='ls-row'>" + node(text, hot=item["position"] == 0) + tag + "</div>"
        ui.card(rows + "<div class='ls-muted'>Top of the stack is shown first.</div>")

    st.markdown("**Try it**")
    left, right = st.columns(2)
    with left:
        _task_form(client, student_id, "stack_push_form", "Push: add revision task")
    with right:
        if st.button("Push: complete next task", key="stack_complete"):
            ui.run_action(lambda: client.complete_task(student_id),
                          lambda done: "Completed " + data.topic_name(done["topic_id"]) + " (undo record pushed).")
        if st.button("Pop: undo last action", disabled=info["size"] == 0, key="stack_pop"):
            ui.run_action(lambda: client.undo(student_id), lambda message: message)


# --------------------------------------------------------- Circular Queue

def _queue(client, student) -> None:
    student_id = student["id"]
    st.subheader("Circular Queue (array)")
    st.write("Purpose: keep pending revision tasks in the order they were added (FIFO). "
             "When the rear reaches the end of the array it wraps around to slot 0 using **(rear + 1) % capacity**.")

    info = client.inspect_queue(student_id)
    columns = st.columns(3)
    columns[0].metric("Front slot", info["front"])
    columns[1].metric("Rear slot", info["rear"])
    columns[2].metric("Tasks waiting", str(info["count"]) + " / " + str(info["capacity"]))

    cells = ""
    for slot in info["slots"]:
        tags = []
        if slot["used"] and slot["slot"] == info["front"]:
            tags.append("FRONT")
        if slot["used"] and slot["slot"] == info["rear"]:
            tags.append("REAR")
        if slot["used"]:
            body = "Task #" + str(slot["task_id"]) + "<br>" + html.escape(data.topic_name(slot["topic_id"])[:14])
            css = "ls-slot"
        else:
            body = "empty"
            css = "ls-slot empty"
        cells += ("<div><div class='ls-slot-label'>" + " + ".join(tags) + "&nbsp;</div>"
                  + "<div class='" + css + "'>" + body + "</div>"
                  + "<div class='ls-slot-label' style='color:#64748B'>slot " + str(slot["slot"]) + "</div></div>")
    ui.card("<div class='ls-row' style='align-items:flex-start'>" + cells + "</div>")

    if info["count"] == 0:
        ui.empty_state("The queue is empty. Enqueue a task to see the front and rear move.")
    st.caption("Tip: add tasks until slot " + str(info["capacity"] - 1) + " is used, complete a few, then add more "
               "to watch the rear wrap around to slot 0.")

    st.markdown("**Try it**")
    left, right = st.columns(2)
    with left:
        _task_form(client, student_id, "queue_enqueue_form", "Enqueue: add task")
    with right:
        if st.button("Dequeue: complete next task", disabled=info["count"] == 0, key="queue_dequeue"):
            ui.run_action(lambda: client.complete_task(student_id),
                          lambda done: "Dequeued " + data.topic_name(done["topic_id"]) + ".")

"""
Clicks through the Streamlit app without a browser (Streamlit's AppTest).
Run from the project folder:   python -m unittest tests.test_ui_flow -v
Build the engine first (see README).
"""
import os
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

# The app must read a throw-away copy of the data, never the real one.
TEMP = Path(tempfile.mkdtemp())
os.environ["LEARNSMART_DATA"] = str(TEMP / "data")

from streamlit_app.engine_client import find_engine_executable   # noqa: E402

try:
    from streamlit.testing.v1 import AppTest
    import streamlit as st
    HAVE_STREAMLIT = True
except ImportError:
    HAVE_STREAMLIT = False

from streamlit_app import data_files as data   # noqa: E402

APP = str(ROOT / "streamlit_app" / "app.py")


def widget(elements, label):
    return [e for e in elements if e.label == label][0]


@unittest.skipIf(not HAVE_STREAMLIT or find_engine_executable() is None, "needs streamlit and the built engine")
class UiFlowTests(unittest.TestCase):
    def setUp(self):
        st.cache_resource.clear()                                         # drops the old engine process
        shutil.rmtree(TEMP / "data", ignore_errors=True)
        shutil.copytree(ROOT / "data", TEMP / "data")
        self.at = AppTest.from_file(APP, default_timeout=60).run()

    def tearDown(self):
        st.cache_resource.clear()

    def login(self, student_id="1001"):
        self.at.sidebar.text_input[0].set_value(student_id)
        self.at.sidebar.button[0].click().run()

    def go(self, section):
        self.at.sidebar.radio(key="section").set_value(section).run()

    def assert_clean(self):
        self.assertEqual([e.value for e in self.at.exception], [])
        self.assertEqual([e.value for e in self.at.error], [])

    # ---------------------------------------------------------------- login
    def test_welcome_then_login(self):
        self.assert_clean()
        self.assertIn("Log in with your student ID", self.at.info[0].value)
        self.login()
        self.assert_clean()
        metrics = {m.label: m.value for m in self.at.metric}
        self.assertEqual(metrics["Quizzes taken"], "4")
        self.assertEqual(metrics["Pending revision tasks"], "2")

    def test_login_errors(self):
        self.login("abc")
        self.assertIn("numbers only", self.at.error[0].value)
        self.login("9999")
        self.assertIn("No student with ID 9999", self.at.error[0].value)

    def test_register_new_student(self):
        self.at.sidebar.text_input[1].set_value("4001")
        self.at.sidebar.text_input[2].set_value("Zainab")
        self.at.sidebar.text_input[3].set_value("z@x.com")
        widget(self.at.sidebar.button, "Register").click().run()
        self.assert_clean()
        self.assertIn("Zainab", self.at.sidebar.markdown[1].value)
        self.assertIn("Take your first quiz", self.at.info[0].value)       # friendly empty state

    # ----------------------------------------------------------------- quiz
    def test_full_quiz_saves_attempt(self):
        self.login()
        self.go("Quiz")
        widget(self.at.button, "Start Quiz").click().run()
        questions = data.load_questions(1)
        self.assertEqual(len(questions), 15)
        for i, q in enumerate(questions):
            right = "ABCD"[q["correct_index"]] + ".  " + q["options"][q["correct_index"]]
            self.at.radio(key="quiz_q_%d" % i).set_value(right)
            self.at.button(key="quiz_next" if i < 14 else "quiz_submit").click().run()
        self.assert_clean()
        self.assertIn("Saved as attempt #11", self.at.success[0].value)
        self.go("Dashboard")
        self.assertEqual({m.label: m.value for m in self.at.metric}["Quizzes taken"], "5")

    def test_submit_needs_every_answer(self):
        self.login()
        self.go("Quiz")
        widget(self.at.button, "Start Quiz").click().run()
        for _ in range(14):
            self.at.button(key="quiz_next").click().run()
        self.at.button(key="quiz_submit").click().run()
        self.assertIn("Please answer question 1", self.at.warning[0].value)

    # ------------------------------------------------------------ dashboard
    def test_revision_buttons(self):
        self.login()
        self.at.button(key="plan_complete").click().run()
        self.assertIn("Completed", self.at.success[0].value)
        self.at.button(key="plan_undo").click().run()
        self.assertIn("Undone", self.at.success[0].value)

    def test_recommended_topic_already_queued(self):
        self.login()
        self.go("Learning Recommendations")
        self.assertTrue(self.at.button(key="rec_add").disabled)            # AVL is already in the queue

    # ------------------------------------------------------------ inspector
    def test_inspector_hash_collision_and_register(self):
        self.login()
        self.go("Data Structure Inspector")
        self.assert_clean()
        metrics = {m.label: m.value for m in self.at.metric}
        self.assertEqual((metrics["Buckets"], metrics["Collisions"]), ("11", "1"))
        widget(self.at.number_input, "New student ID").set_value(1023)     # 1023 % 11 = 0 (shares bucket 0 with 1001 and 1012)
        widget(self.at.text_input, "Name").set_value("Collision Test")
        widget(self.at.button, "Register student (insert)").click().run()
        self.assertIn("registered", self.at.success[0].value)
        self.assertEqual({m.label: m.value for m in self.at.metric}["Collisions"], "2")

    def test_inspector_linked_list_insert_and_delete(self):
        self.login()
        self.go("Data Structure Inspector")
        widget(self.at.button, "Insert at tail").click().run()
        self.assertIn("at the tail", self.at.success[0].value)
        self.at.button(key="ll_delete").click().run()
        self.assertIn("Deleted attempt", self.at.success[0].value)

    def test_inspector_queue_and_stack(self):
        self.login("1003")
        self.go("Data Structure Inspector")
        self.assertEqual({m.label: m.value for m in self.at.metric}["Tasks waiting"], "0 / 10")
        widget(self.at.button, "Enqueue: add task").click().run()
        self.assert_clean()
        self.assertEqual({m.label: m.value for m in self.at.metric}["Tasks waiting"], "1 / 10")
        self.at.button(key="queue_dequeue").click().run()
        self.assertIn("Dequeued", self.at.success[0].value)
        self.at.button(key="stack_pop").click().run()
        self.assertIn("Undone", self.at.success[0].value)

    def test_remove_logged_in_student_logs_out(self):
        self.login()
        self.go("Data Structure Inspector")
        widget(self.at.number_input, "Student ID to remove").set_value(1001)
        self.at.button(key="hash_remove").click().run()
        self.assertIn("Log in with your student ID", self.at.info[0].value)


if __name__ == "__main__":
    unittest.main()

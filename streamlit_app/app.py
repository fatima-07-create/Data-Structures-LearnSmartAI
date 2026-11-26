"""
LearnSmart - Streamlit entry file.
Start it from the project folder with:   streamlit run streamlit_app/app.py
"""
import sys
from pathlib import Path

# Lets "from streamlit_app import ..." work no matter where Streamlit is started from.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import streamlit as st

from streamlit_app import ui_helpers as ui
from streamlit_app.engine_client import DATA_FOLDER, EngineClient, EngineError, find_engine_executable
from streamlit_app.sections import dashboard, inspector, quiz, recommendations

SECTIONS = ["Dashboard", "Quiz", "Learning Recommendations", "Data Structure Inspector"]

st.set_page_config(page_title="LearnSmart", page_icon=":mortar_board:", layout="wide")
ui.load_css()


@st.cache_resource
def get_client(executable: str, data_folder: str) -> EngineClient:
    """One engine process is shared by all reruns of the app."""
    return EngineClient(Path(executable), Path(data_folder))


def connect():
    executable = find_engine_executable()
    if executable is None:
        st.error("The C++ engine was not found.")
        st.write("Build it first, then reload this page:")
        st.markdown("- **Visual Studio:** open `LearnSmart.sln`, choose **x64** and press **Ctrl+Shift+B**.\n"
                    "- **Terminal:** `g++ -std=c++17 src/*.cpp -o build/LearnSmart`\n"
                    "- Or set the environment variable `LEARNSMART_ENGINE` to the full path of the program.")
        st.stop()
    return get_client(str(executable), str(DATA_FOLDER))


# ------------------------------------------------------------------ sidebar

def login_box(client) -> None:
    st.sidebar.markdown("**Log in with your student ID**")
    with st.sidebar.form("login_form"):
        raw_id = st.text_input("Student ID", placeholder="for example 1001")
        pressed = st.form_submit_button("Log in", type="primary")
    if pressed:
        try:
            student_id = int(raw_id.strip())
        except ValueError:
            ui.flash("error", "Please type your student ID using numbers only.")
            st.rerun()
        student = client.get_student(student_id)
        if student is None:
            ui.flash("error", "No student with ID " + str(student_id) + ". Register below if you are new.")
        else:
            st.session_state["student_id"] = student_id
        st.rerun()

    with st.sidebar.expander("New student? Register"):
        with st.form("register_form"):
            new_id = st.text_input("Choose a student ID (numbers only)")
            name = st.text_input("Name")
            email = st.text_input("Email")
            registered = st.form_submit_button("Register", type="primary")
        if registered:
            try:
                student_id = int(new_id.strip())
                client.register_student(student_id, name, email)
            except ValueError:
                ui.flash("error", "The student ID must be a number.")
            except EngineError as error:
                ui.flash("error", str(error))
            else:
                st.session_state["student_id"] = student_id
                ui.flash("success", "Welcome, " + name.strip() + "! Your account is ready.")
            st.rerun()


def sidebar(client, student) -> str:
    st.sidebar.markdown("<h2 style='color:#172554;margin-bottom:0'>LearnSmart</h2>", unsafe_allow_html=True)
    st.sidebar.caption("AI-powered personalized learning assistant")

    if student is None:
        login_box(client)
        return SECTIONS[0]

    st.sidebar.markdown("Logged in as **" + student["name"] + "** (ID " + str(student["id"]) + ")")
    if st.sidebar.button("Log out"):
        st.session_state.pop("student_id", None)
        st.session_state.pop("quiz", None)
        st.rerun()
    st.sidebar.divider()
    return st.sidebar.radio("Section", SECTIONS, key="section")


def welcome(client) -> None:
    ui.page_title("LearnSmart", "Your personal study assistant for Data Structures, OOP, Database, OS and Programming Fundamentals.")
    ui.empty_state("Log in with your student ID in the sidebar to start.")
    try:
        students = client.list_students()
    except EngineError:
        students = []
    if students:
        st.caption("Demo student IDs: " + ", ".join(str(s["id"]) for s in students[:6]))


# --------------------------------------------------------------------- main

def main() -> None:
    client = connect()
    try:
        student = None
        if "student_id" in st.session_state:
            student = client.get_student(st.session_state["student_id"])
            if student is None:                     # the student was removed in the Inspector
                st.session_state.pop("student_id", None)

        section = sidebar(client, student)
        ui.show_flash()

        if student is None:
            welcome(client)
        elif section == "Dashboard":
            dashboard.render(client, student)
        elif section == "Quiz":
            quiz.render(client, student)
        elif section == "Learning Recommendations":
            recommendations.render(client, student)
        else:
            inspector.render(client, student)
    except EngineError as error:
        st.error("Something went wrong with the engine: " + str(error))


main()

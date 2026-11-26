@echo off
REM Starts the LearnSmart Streamlit app. Build the C++ engine first (Visual Studio: Ctrl+Shift+B, x64).
cd /d "%~dp0"
python -m pip install -r requirements.txt
python -m streamlit run streamlit_app/app.py

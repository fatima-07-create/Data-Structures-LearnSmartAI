#!/usr/bin/env bash
# Starts the LearnSmart Streamlit app. Build the C++ engine first:
#   mkdir -p build && g++ -std=c++17 src/*.cpp -o build/LearnSmart
cd "$(dirname "$0")"
python3 -m pip install -r requirements.txt
python3 -m streamlit run streamlit_app/app.py

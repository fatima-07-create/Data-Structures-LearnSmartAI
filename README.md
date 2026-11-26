# LearnSmart - Member A (C++ engine + Streamlit app)

**Member A data structures:** Hash Table, Singly Linked List, Stack, Circular Queue (all manual C++).
**Member A modules:** student records, quiz-history storage, revision planner, undo, CSV persistence,
Python bridge, Dashboard, quiz history, revision queue, and the Inspector tabs for the four structures.
Member B plugs in BST, AVL, Heap, Graph, recommendations and the rest of the Inspector.

## 1. Build the C++ engine (Visual Studio)
1. Double-click **LearnSmart.sln**.
2. Choose **x64** and **Debug**, then press **Ctrl+Shift+B** (or F5 to try the text menu).
3. The program is created at `bin\x64\Debug\LearnSmart.exe`. The Python app finds it by itself.

If Visual Studio asks to "retarget" the project, click OK. You need the "Desktop development with C++" workload.
No Visual Studio? `mkdir build` then `g++ -std=c++17 src/*.cpp -o build/LearnSmart` works too.
You can also point the app to any build with the environment variable `LEARNSMART_ENGINE`.

## 2. Run the Streamlit app
```
run_app.bat            (Windows)         or         ./run_app.sh
```
This installs Streamlit (`requirements.txt`) and starts `streamlit run streamlit_app/app.py`. Log in with a demo ID:
**1001** (Ayesha, mixed results), **1002** (Bilal, declining Stack score), **1003** (Sara, empty revision queue), **1012** (Hamza, collides with 1001 in the hash table).

| Section | What works now |
|---|---|
| Dashboard | metric cards, next topic suggestion with reason, topic chart, topic table, quiz history (filter, delete, add), revision planner (add, complete, undo) |
| Quiz | Subject, then sub-topic, 15 MCQs one per screen, score, review, saved to the Linked List |
| Learning Recommendations | basic recommendation with reason, notes, key points, worked example, practice, resources, add to revision queue |
| Data Structure Inspector | Hash Table, Linked List, Stack, Circular Queue are live; BST, AVL, Heap, Graph are reserved for Member B |

Prerequisite-aware recommendations (Graph + Heap) and scoring inside C++ are Member B's modules. The screens already have a spot for them.

## 3. Start modes of the C++ program
| Command | What it does |
|---|---|
| `LearnSmart.exe` | text menu (16 options, includes the inspector and the self tests) |
| `LearnSmart.exe --engine` | command mode used by the Python bridge (see docs/PROTOCOL.md) |
| `LearnSmart.exe --test` | 159 self checks (exit code 0 = all passed) |
| `--data <folder>` | use another data folder |

## 4. Tests
```
LearnSmart.exe --test                      C++ structures, manager, CSV, command runner
python -m unittest tests.test_bridge -v    Python bridge + engine (8 tests)
python -m unittest tests.test_ui_flow -v   clicks through the app without a browser (11 tests)
```
Results are written in docs/TEST_REPORT.md.

## 5. Project map
| Path | Job |
|---|---|
| `src/StudentHashTable.*` `QuizHistoryList.*` `UndoStack.*` `RevisionQueue.*` | the four structures |
| `src/Student.h` | one student (owns a list, a queue and a stack) |
| `src/StudentManager.*` | all checked actions (register, quiz, revision, undo) |
| `src/CsvStorage.*` | save / load CSV |
| `src/CommandRunner.*` | `--engine` mode |
| `src/ConsoleMenu.*` `SelfTests.*` `main.cpp` | text menu, tests, entry point |
| `streamlit_app/engine_client.py` | the Python bridge |
| `streamlit_app/app.py` | login, sidebar, navigation |
| `streamlit_app/sections/` | dashboard, quiz, recommendations, inspector |
| `streamlit_app/theme/style.css` and `.streamlit/config.toml` | theme from blueprint section 13 |
| `data/` | CSV files (see below) |

## 6. Data files
34 sub-topics in 5 subjects, 15 MCQs each (510 questions). Topic IDs are fixed, keep them the same in every file.

| Subject | Topic IDs | Sub-topics |
|---|---|---|
| Data Structures | 1-9 | Arrays, Linked List, Stack, Queue, Hash Table, Binary Search Tree, AVL Tree, Heap, Graph |
| OOP | 10-14 | Classes and Objects, Encapsulation, Abstraction, Inheritance, Polymorphism |
| Database | 15-20 | Relational Model and Keys, ER Modeling, SQL Basics, Joins and Queries, Normalization, Transactions and ACID |
| Operating Systems | 21-27 | Processes and Threads, CPU Scheduling, Synchronization, Deadlocks, Memory Management, Virtual Memory, File Systems |
| Programming Fundamentals | 28-34 | Variables and Data Types, Operators and Expressions, Conditional Statements, Loops, Functions, Arrays and Strings, Pointers and References |

| File | Columns | Written by |
|---|---|---|
| `students.csv` | student_id, name, email | C++ engine |
| `quiz_results.csv` | attempt_id, student_id, topic_id, score, total_marks, date | C++ engine |
| `revision_tasks.csv` | task_id, student_id, topic_id, status, priority | C++ engine |
| `topics.csv` | topic_id, **subject**, topic_name, difficulty, short_description | content (subject column is new) |
| `questions.csv` | question_id, topic_id, question, options, correct_answer, difficulty | content |
| `tutor_notes.csv` | topic_id, summary, key_points, worked_example, common_mistakes, practice_questions | content |
| `learning_resources.csv` | topic_id, resource_title, resource_type, resource_url | content (open each link once before the demo) |

Rules: no commas inside a cell; `options` has 4 choices separated by `;` and `correct_answer` is A to D;
`key_points` and `practice_questions` use `;` between items.
`prerequisites.csv` (topic_id, prerequisite_topic_id) belongs to Member B's Graph.

## 7. Notes for Member B
- C++: `manager.findStudent(id)` gives a student; `student->quizHistory.head()` starts the quiz list (walk with `->next`).
- Add new commands to `CommandRunner` and document them in `docs/PROTOCOL.md`; add matching helpers in `engine_client.py`.
- `streamlit_app/ui_helpers.py` has a display-only `topic_performance()` (average, band, trend). Replace it with B's engine command when ready.
- The Quiz screen checks answers in Python for now. Moving scoring into C++ only changes `_finish()` in `sections/quiz.py`.

## 8. Viva notes (Member A)
- **Hash function:** `id % bucketCount`. **Collision:** two IDs in one bucket (try 1001 and 1012); chaining keeps both in a linked chain. **Rehash:** when more than 75% full, buckets become `2n+1` and every node is re-placed.
- **Linked list:** tail pointer makes insert O(1); delete needs the node *before* the target.
- **Stack (LIFO):** the last revision action is undone first.
- **Circular queue (FIFO):** `(index + 1) % capacity` wraps around; the Inspector shows front, rear and wraparound.
- **Undo:** "add task" is undone by removing from the rear; "complete task" is undone by putting the task back at the front.
- **Persistence:** the engine loads the CSV files at start and saves after every change. Undo history is not saved.
- **Python to C++:** Python starts `LearnSmart --engine`, writes one command per line and reads one reply (`OK|...` or `ERROR|...`).

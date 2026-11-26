# Test report - Member A

Date: 25 Nov 2026. Environment: g++ 17 standard with `-Wall -Wextra -Wpedantic` (no warnings) and AddressSanitizer + UBSan (no findings); Python 3 with Streamlit 1.67.

| Suite | How to run | Result |
|---|---|---|
| C++ self tests | `LearnSmart --test` | 159 of 159 checks passed |
| Python bridge + engine | `python -m unittest tests.test_bridge` | 8 of 8 passed |
| Streamlit app flows | `python -m unittest tests.test_ui_flow` | 11 of 11 passed |

## What the C++ checks cover (blueprint 9.1)
- **Hash Table:** collision (1001 and 1012), duplicate ID, missing ID, delete, rehash (30 students into 3 buckets, all found).
- **Linked List:** empty list, one node, many nodes, search, delete head / middle / tail, insert after deleting the tail.
- **Stack:** pop and peek on empty, push to capacity, push on full, LIFO order, clear.
- **Circular Queue:** empty and full cases, FIFO order, wraparound, insert at front, remove from rear, reuse after emptying.
- **Manager:** validation (IDs, scores, dates, priority, duplicate topic), complete / undo of every action type, undo on empty.
- **CSV:** save then load into a new manager, order kept, ID counters continue after load.
- **Command runner:** every command, wrong arguments, unknown command, inspector replies.

## What the Python tests cover
- Bridge parsing of every reply type, friendly errors, pipe cleaning, data surviving a restart, engine restart after a crash.
- App flows: welcome, login errors, register, full 15-question quiz saved to history, submit blocked until all answered, revision complete / undo, queued topic cannot be added twice, hash collision demo, linked list insert / delete, queue and stack buttons, logout after the logged-in student is removed.

## Known limits
- Quiz answers are checked in Python (Member B may move this into C++).
- Recommendations on the Dashboard and Recommendations screens are basic (weakest topic); prerequisite-aware logic belongs to Member B.
- The BST, AVL, Heap and Graph Inspector tabs are placeholders until Member B's structures exist.
- Resource links were not opened automatically; check them once.
- The app was tested headless (Streamlit AppTest) and the Visual Studio project could not be opened here, so the first build in Visual Studio is the final check.

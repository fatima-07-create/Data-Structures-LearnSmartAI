// SelfTests.cpp
// Simple tests with no test framework: each check() prints only if it fails, then we show a summary.
#include "SelfTests.h"
#include "CommandRunner.h"
#include "CsvStorage.h"
#include "QuizHistoryList.h"
#include "RevisionQueue.h"
#include "StudentHashTable.h"
#include "StudentManager.h"
#include "UndoStack.h"
#include <filesystem>
#include <iostream>

static int totalChecks = 0;
static int failedChecks = 0;

static void check(bool condition, const std::string& description) {
    totalChecks++;
    if (!condition) {
        failedChecks++;
        std::cout << "  FAILED: " << description << "\n";
    }
}

static QuizAttempt makeAttempt(int id, int topic) {
    QuizAttempt a;
    a.attemptId = id;
    a.studentId = 1;
    a.topicId = topic;
    a.score = 5;
    a.totalMarks = 10;
    a.date = "2026-01-01";
    return a;
}

static RevisionTask makeTask(int id, int topic) {
    RevisionTask t;
    t.taskId = id;
    t.topicId = topic;
    t.priority = 50;
    return t;
}

// ------------------------------------------------------------- Hash Table

static void testHashTable() {
    std::cout << "Hash Table...\n";
    StudentHashTable table(11);

    check(table.search(1001) == nullptr, "empty table: search finds nothing");

    // 1001 and 1012 land in the same bucket when there are 11 buckets (collision).
    check(table.insert(new Student(1001, "Ayesha", "a@x.com")), "insert 1001");
    check(table.insert(new Student(1012, "Hamza", "h@x.com")), "insert 1012 (collision)");
    check(table.collisionCount() == 1, "one collision is counted");
    check(table.search(1001) != nullptr && table.search(1012) != nullptr, "both colliding students are found");

    Student* duplicate = new Student(1001, "Copy", "c@x.com");
    check(!table.insert(duplicate), "duplicate ID is rejected");
    delete duplicate;
    check(table.studentCount() == 2, "count is still 2 after duplicate");

    check(table.remove(1012), "remove the second student in the chain");
    check(table.search(1012) == nullptr, "removed student is gone");
    check(table.search(1001) != nullptr, "other student is still there");
    check(!table.remove(9999), "removing a missing ID returns false");
    check(table.remove(1001), "remove the last student");
    check(table.studentCount() == 0, "table is empty again");

    // Rehash: start tiny and add many students.
    StudentHashTable small(3);
    for (int id = 1; id <= 30; id++) small.insert(new Student(id, "S", "e"));
    check(small.bucketCount() > 3, "table grew (rehash happened)");
    bool allFound = true;
    for (int id = 1; id <= 30; id++) {
        if (small.search(id) == nullptr) allFound = false;
    }
    check(allFound, "all 30 students are found after rehash");
    check(small.studentCount() == 30, "count is 30 after rehash");
}

// ------------------------------------------------------------ Linked List

static void testLinkedList() {
    std::cout << "Linked List...\n";
    QuizHistoryList list;

    check(list.isEmpty() && list.size() == 0, "new list is empty");
    check(!list.deleteByAttemptId(1), "delete on empty list returns false");
    check(list.overallAveragePercent() == 0.0, "average of empty list is 0");

    list.insertAtTail(makeAttempt(1, 10));
    check(list.size() == 1 && list.head() != nullptr, "one node inserted");
    check(list.deleteByAttemptId(1) && list.isEmpty(), "delete the only node");

    for (int i = 1; i <= 5; i++) list.insertAtTail(makeAttempt(i, i % 2 == 0 ? 20 : 10));
    check(list.size() == 5, "five nodes inserted");
    check(list.searchByAttemptId(3) != nullptr, "search finds attempt 3");
    check(list.searchByAttemptId(99) == nullptr, "search misses attempt 99");
    check(list.countForTopic(10) == 3 && list.countForTopic(20) == 2, "topic counts are right");
    check(list.findLatestForTopic(10)->attemptId == 5, "latest attempt for topic 10 is #5");

    check(list.deleteByAttemptId(1), "delete head");
    check(list.deleteByAttemptId(3), "delete middle");
    check(list.deleteByAttemptId(5), "delete tail");
    check(list.size() == 2, "two nodes remain");

    list.insertAtTail(makeAttempt(6, 10));    // tail pointer must still be correct
    check(list.size() == 3 && list.searchByAttemptId(6) != nullptr, "insert after deleting tail works");
    check(list.overallAveragePercent() == 50.0, "average is 50%");
}

// ------------------------------------------------------------------ Stack

static void testStack() {
    std::cout << "Stack...\n";
    UndoStack stack;
    UndoAction action;

    check(stack.isEmpty() && !stack.isFull(), "new stack is empty");
    check(!stack.pop(action), "pop from empty stack fails");
    check(!stack.peek(action), "peek on empty stack fails");

    for (int i = 1; i <= MAX_UNDO_ACTIONS; i++) {
        UndoAction a;
        a.task = makeTask(i, i);
        check(stack.push(a), "push within capacity");
    }
    check(stack.isFull(), "stack is full");
    UndoAction extra;
    check(!stack.push(extra), "push to a full stack fails");

    check(stack.peek(action) && action.task.taskId == MAX_UNDO_ACTIONS, "peek shows the last pushed item");
    check(stack.pop(action) && action.task.taskId == MAX_UNDO_ACTIONS, "pop returns the last pushed item (LIFO)");
    check(stack.pop(action) && action.task.taskId == MAX_UNDO_ACTIONS - 1, "next pop is the one before");
    check(stack.size() == MAX_UNDO_ACTIONS - 2, "size went down by two");
    stack.clear();
    check(stack.isEmpty(), "clear empties the stack");
}

// --------------------------------------------------------- Circular Queue

static void testQueue() {
    std::cout << "Circular Queue...\n";
    RevisionQueue queue;
    RevisionTask task;

    check(queue.isEmpty() && !queue.isFull(), "new queue is empty");
    check(!queue.dequeue(task), "dequeue from empty queue fails");
    check(!queue.front(task), "front of empty queue fails");

    for (int i = 1; i <= MAX_REVISION_TASKS; i++) check(queue.enqueue(makeTask(i, i)), "enqueue within capacity");
    check(queue.isFull(), "queue is full");
    check(!queue.enqueue(makeTask(99, 99)), "enqueue to a full queue fails");
    check(queue.front(task) && task.taskId == 1, "front is the first task (FIFO)");

    // Wraparound: free 3 slots at the front, then add 3 more at the rear.
    for (int i = 1; i <= 3; i++) check(queue.dequeue(task) && task.taskId == i, "dequeue keeps FIFO order");
    for (int i = 11; i <= 13; i++) check(queue.enqueue(makeTask(i, i)), "enqueue after wraparound");
    check(queue.size() == MAX_REVISION_TASKS, "queue is full again");
    check(queue.getAt(0, task) && task.taskId == 4, "position 0 is task 4");
    check(queue.getAt(MAX_REVISION_TASKS - 1, task) && task.taskId == 13, "last position is task 13");
    check(queue.contains(12) && !queue.contains(1), "contains works across the wrap");

    // The two extra moves used by undo.
    check(queue.removeFromRear(task) && task.taskId == 13, "removeFromRear returns the newest task");
    check(queue.insertAtFront(makeTask(3, 3)), "insertAtFront puts a task back");
    check(queue.front(task) && task.taskId == 3, "front is the task we just put back");

    while (queue.dequeue(task)) {}
    check(queue.isEmpty(), "queue can be emptied completely");
    check(queue.enqueue(makeTask(50, 50)) && queue.front(task) && task.taskId == 50, "queue still works after being emptied");
}

// ---------------------------------------------------------------- Manager

static void testManager() {
    std::cout << "Student manager (undo, validation)...\n";
    StudentManager manager;

    check(manager.registerStudent(1001, "Ayesha", "a@x.com"), "register student");
    check(!manager.registerStudent(1001, "Again", "b@x.com"), "duplicate student rejected");
    check(!manager.registerStudent(-5, "Bad", ""), "negative ID rejected");
    check(!manager.registerStudent(5, "   ", ""), "empty name rejected");

    check(manager.addQuizAttempt(1001, 1, 8, 10, "2026-10-01") == 1, "first attempt gets ID 1");
    check(manager.addQuizAttempt(1001, 1, 11, 10, "2026-10-01") == -1, "score above total rejected");
    check(manager.addQuizAttempt(1001, 1, 5, 10, "01-10-2026") == -1, "bad date rejected");
    check(manager.addQuizAttempt(4040, 1, 5, 10, "2026-10-01") == -1, "unknown student rejected");
    check(manager.deleteQuizAttempt(1001, 1), "delete attempt");
    check(!manager.deleteQuizAttempt(1001, 1), "deleting the same attempt twice fails");

    std::string message;
    RevisionTask done;
    check(!manager.undoLastAction(1001, message), "undo with nothing to undo fails");
    check(!manager.completeNextTask(1001, done), "complete with empty queue fails");

    int first  = manager.addRevisionTask(1001, 7, 80);
    int second = manager.addRevisionTask(1001, 2, 50);
    check(first == 1 && second == 2, "task IDs are 1 and 2");
    check(manager.addRevisionTask(1001, 7, 10) == -1, "same topic twice is rejected");
    check(manager.addRevisionTask(1001, 3, 150) == -1, "priority above 100 rejected");

    Student* s = manager.findStudent(1001);
    check(s->revisionQueue.size() == 2, "two tasks are waiting");

    check(manager.completeNextTask(1001, done) && done.taskId == 1 && done.status == "Done", "complete takes the FRONT task");
    check(s->revisionQueue.size() == 1, "one task left");

    check(manager.undoLastAction(1001, message), "undo the completion");
    check(s->revisionQueue.size() == 2, "task is back in the queue");
    RevisionTask front;
    s->revisionQueue.front(front);
    check(front.taskId == 1, "restored task is at the FRONT again");

    check(manager.undoLastAction(1001, message), "undo the second add");
    check(s->revisionQueue.size() == 1, "second task removed again");
    check(manager.undoLastAction(1001, message), "undo the first add");
    check(s->revisionQueue.isEmpty(), "queue is empty after all undos");
    check(!manager.undoLastAction(1001, message), "nothing left to undo");

    check(manager.removeStudent(1001), "remove student");
    check(!manager.removeStudent(1001), "removing twice fails");
}

// -------------------------------------------------------------------- CSV

static void testCsv() {
    std::cout << "CSV save and load...\n";
    std::string folder = (std::filesystem::temp_directory_path() / "learnsmart_selftest").string();
    std::error_code ec;
    std::filesystem::remove_all(folder, ec);

    StudentManager original;
    original.registerStudent(1001, "Ayesha Khan", "ayesha@x.com");
    original.registerStudent(1002, "Bilal", "bilal@x.com");
    original.addQuizAttempt(1001, 2, 3, 10, "2026-10-01");
    original.addQuizAttempt(1001, 2, 6, 10, "2026-10-08");
    original.addQuizAttempt(1002, 5, 9, 10, "2026-10-02");
    original.addRevisionTask(1001, 7, 80);
    original.addRevisionTask(1001, 2, 50);

    CsvStorage storage(folder);
    check(storage.save(original), "save succeeds");

    StudentManager loaded;
    CsvStorage reader(folder);
    check(reader.load(loaded), "load succeeds");
    check(reader.skippedRows() == 0, "no rows skipped");
    check(loaded.students().studentCount() == 2, "both students loaded");

    Student* s = loaded.findStudent(1001);
    check(s != nullptr && s->name == "Ayesha Khan", "student details loaded");
    check(s != nullptr && s->quizHistory.size() == 2, "quiz history loaded");
    check(s != nullptr && s->revisionQueue.size() == 2, "revision queue loaded");

    RevisionTask front;
    check(s != nullptr && s->revisionQueue.front(front) && front.topicId == 7, "queue order kept");
    check(loaded.addQuizAttempt(1001, 3, 5, 10, "2026-10-10") == 4, "next attempt ID continues after load");
    check(loaded.addRevisionTask(1002, 4, 20) == 3, "next task ID continues after load");

    std::filesystem::remove_all(folder, ec);
}

// ------------------------------------------------------ command runner

static bool startsWith(const std::string& text, const std::string& start) {
    return text.compare(0, start.size(), start) == 0;
}

static bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

static void testCommands() {
    std::cout << "Command runner (engine mode)...\n";
    std::string folder = (std::filesystem::temp_directory_path() / "learnsmart_cmdtest").string();
    std::error_code ec;
    std::filesystem::remove_all(folder, ec);

    StudentManager manager;
    CsvStorage storage(folder);
    CommandRunner runner(manager, storage);
    bool keepGoing = true;
    auto run = [&](const std::string& line) { return runner.execute(line, keepGoing); };

    check(run("PING") == "OK|pong", "PING answers pong");
    check(startsWith(run("NOPE|1"), "ERROR|Unknown command"), "unknown command gives an error");
    check(startsWith(run("ADD_STUDENT|abc|Ali|a@x.com"), "ERROR|Wrong arguments"), "bad student ID gives an error");
    check(run("ADD_STUDENT|1001|Ayesha|a@x.com") == "OK|Student registered", "ADD_STUDENT works");
    check(run("ADD_STUDENT|1012|Hamza|h@x.com") == "OK|Student registered", "second student works");
    check(startsWith(run("ADD_STUDENT|1001|Again|b@x.com"), "ERROR|"), "duplicate student gives an error");

    check(run("ADD_ATTEMPT|1001|2|7|10|2026-11-01") == "OK|1", "ADD_ATTEMPT returns attempt ID 1");
    check(run("ADD_ATTEMPT|1001|2|9|10|2026-11-02") == "OK|2", "second attempt returns ID 2");
    check(startsWith(run("ADD_ATTEMPT|1001|2|99|10|2026-11-02"), "ERROR|"), "score above total gives an error");
    check(run("GET_STUDENT|1001") == "OK|1001|Ayesha|a@x.com|2|0|0|80.0", "GET_STUDENT shows counts and average");
    check(startsWith(run("LIST_ATTEMPTS|1001"), "OK|2\nROW|1|2|7|10|2026-11-01"), "LIST_ATTEMPTS lists in order");
    check(startsWith(run("SEARCH_TOPIC|1001|2"), "OK|2\n"), "SEARCH_TOPIC finds both attempts");
    check(run("SEARCH_TOPIC|1001|9") == "OK|0\nEND", "SEARCH_TOPIC for an unknown topic is empty");

    check(run("ADD_TASK|1001|7|80") == "OK|1", "ADD_TASK returns task ID 1");
    check(startsWith(run("INSPECT_STACK|1001"), "OK|1|20\nROW|0|AddTask|1|7"), "stack shows the add action on top");
    check(run("COMPLETE_TASK|1001") == "OK|1|7", "COMPLETE_TASK takes the front task");
    check(startsWith(run("INSPECT_STACK|1001"), "OK|2|20\nROW|0|CompleteTask|1|7"), "stack top is now the complete action");
    check(startsWith(run("UNDO|1001"), "OK|Undone: Completed task #1"), "UNDO reverses the completion");
    check(startsWith(run("INSPECT_QUEUE|1001"), "OK|0|0|1|10\nROW|0|1|1|7|80"), "queue shows front 0 rear 0 and one task");
    check(startsWith(run("LIST_TASKS|1001"), "OK|1\nROW|1|7|Pending|80"), "LIST_TASKS shows the pending task");

    check(run("HASH_INDEX|1001") == "OK|0|11" && run("HASH_INDEX|1012") == "OK|0|11", "1001 and 1012 share bucket 0");
    check(startsWith(run("INSPECT_HASH"), "OK|11|2|1\n"), "INSPECT_HASH reports one collision");
    check(contains(run("INSPECT_HASH"), "ROW|0|1|1001|Ayesha"), "INSPECT_HASH lists the chain (1001 is second after insert)");
    check(startsWith(run("LIST_STUDENTS"), "OK|2\n"), "LIST_STUDENTS counts both students");

    check(run("DELETE_ATTEMPT|1001|1") == "OK|Attempt deleted", "DELETE_ATTEMPT works");
    check(run("REMOVE_STUDENT|1012") == "OK|Student removed", "REMOVE_STUDENT works");
    check(run("SAVE") == "OK|saved", "SAVE works");
    check(run("EXIT") == "OK|bye" && !keepGoing, "EXIT stops the loop");

    std::filesystem::remove_all(folder, ec);
}

// ------------------------------------------------------------------- main

int runSelfTests() {
    totalChecks = 0;
    failedChecks = 0;

    std::cout << "Running LearnSmart self tests...\n";
    testHashTable();
    testLinkedList();
    testStack();
    testQueue();
    testManager();
    testCsv();
    testCommands();

    std::cout << "\n" << (totalChecks - failedChecks) << " of " << totalChecks << " checks passed.\n";
    if (failedChecks == 0) std::cout << "ALL TESTS PASSED\n";
    else std::cout << failedChecks << " CHECK(S) FAILED\n";
    return failedChecks;
}

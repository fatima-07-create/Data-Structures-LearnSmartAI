// ConsoleMenu.cpp
#include "ConsoleMenu.h"
#include "SelfTests.h"
#include "Utils.h"
#include <iostream>

// ---------------------------------------------------------- input helpers

static std::string askText(const std::string& question) {
    std::cout << question << ": ";
    std::string answer;
    std::getline(std::cin, answer);
    return trimText(answer);
}

// Keeps asking until the user types a whole number. Returns false only if input has ended.
static bool askInt(const std::string& question, int& number) {
    while (true) {
        std::cout << question << ": ";
        std::string answer;
        if (!std::getline(std::cin, answer)) return false;
        if (toInt(answer, number)) return true;
        std::cout << "  Please type a whole number.\n";
    }
}

// Asks for a student ID and finds the student. Returns nullptr (with a message) if not found.
static Student* askForStudent(const StudentManager& manager) {
    int id = 0;
    if (!askInt("Student ID", id)) return nullptr;
    Student* student = manager.findStudent(id);
    if (student == nullptr) std::cout << "  Student not found.\n";
    return student;
}

// ------------------------------------------------------------------- menu

ConsoleMenu::ConsoleMenu(StudentManager& studentManager, CsvStorage& csvStorage)
    : manager(studentManager), storage(csvStorage) {}

void ConsoleMenu::printMenu() const {
    std::cout << "\n========== LEARNSMART  (Member A) ==========\n"
              << " STUDENTS (Hash Table)\n"
              << "  1. Register student\n"
              << "  2. Find student\n"
              << "  3. Remove student\n"
              << " QUIZ HISTORY (Linked List)\n"
              << "  4. Add quiz attempt\n"
              << "  5. Show quiz history\n"
              << "  6. Search history by topic\n"
              << "  7. Delete quiz attempt\n"
              << " REVISION PLANNER (Queue + Stack)\n"
              << "  8. Add revision task\n"
              << "  9. Show revision queue\n"
              << " 10. Complete next task\n"
              << " 11. Undo last action\n"
              << " INSPECTOR\n"
              << " 12. Show hash table buckets\n"
              << " 13. Show undo stack\n"
              << " 14. Show circular queue slots\n"
              << " OTHER\n"
              << " 15. Save now\n"
              << " 16. Run self tests\n"
              << "  0. Save and exit\n";
}

void ConsoleMenu::run() {
    while (true) {
        printMenu();
        int choice = 0;
        if (!askInt("Choose an option", choice)) break;     // input ended

        if      (choice == 1)  registerStudent();
        else if (choice == 2)  findStudent();
        else if (choice == 3)  removeStudent();
        else if (choice == 4)  addQuizAttempt();
        else if (choice == 5)  showQuizHistory();
        else if (choice == 6)  searchHistoryByTopic();
        else if (choice == 7)  deleteQuizAttempt();
        else if (choice == 8)  addRevisionTask();
        else if (choice == 9)  showRevisionQueue();
        else if (choice == 10) completeNextTask();
        else if (choice == 11) undoLastAction();
        else if (choice == 12) showHashTable();
        else if (choice == 13) showUndoStack();
        else if (choice == 14) showQueueSlots();
        else if (choice == 15) saveNow();
        else if (choice == 16) runSelfTests();
        else if (choice == 0)  break;
        else std::cout << "  Unknown option. Try again.\n";
    }

    if (storage.save(manager)) std::cout << "Data saved. Goodbye!\n";
    else std::cout << "Could not save: " << storage.lastError() << "\n";
}

// -------------------------------------------------------------- students

void ConsoleMenu::registerStudent() {
    int id = 0;
    if (!askInt("New student ID (numbers only)", id)) return;
    std::string name  = askText("Name");
    std::string email = askText("Email");

    if (manager.registerStudent(id, name, email)) std::cout << "  Student registered.\n";
    else std::cout << "  Error: " << manager.lastError() << "\n";
}

void ConsoleMenu::findStudent() {
    Student* s = askForStudent(manager);
    if (s == nullptr) return;
    std::cout << "  ID: " << s->id << "\n  Name: " << s->name << "\n  Email: " << s->email << "\n"
              << "  Quiz attempts: " << s->quizHistory.size() << "\n"
              << "  Pending revision tasks: " << s->revisionQueue.size() << "\n";
    if (!s->quizHistory.isEmpty()) {
        std::cout << "  Overall average: " << static_cast<int>(s->quizHistory.overallAveragePercent() + 0.5) << "%\n";
    }
}

void ConsoleMenu::removeStudent() {
    int id = 0;
    if (!askInt("Student ID to remove", id)) return;
    if (manager.removeStudent(id)) std::cout << "  Student removed.\n";
    else std::cout << "  Error: " << manager.lastError() << "\n";
}

// ----------------------------------------------------------- quiz history

void ConsoleMenu::addQuizAttempt() {
    int studentId = 0, topicId = 0, score = 0, total = 0;
    if (!askInt("Student ID", studentId)) return;
    if (!askInt("Topic ID", topicId)) return;
    if (!askInt("Score obtained", score)) return;
    if (!askInt("Total marks", total)) return;
    std::string date = askText("Date (YYYY-MM-DD)");

    int attemptId = manager.addQuizAttempt(studentId, topicId, score, total, date);
    if (attemptId >= 0) std::cout << "  Saved as attempt #" << attemptId << ".\n";
    else std::cout << "  Error: " << manager.lastError() << "\n";
}

void ConsoleMenu::showQuizHistory() {
    Student* s = askForStudent(manager);
    if (s == nullptr) return;
    std::cout << "  Quiz history for " << s->name << ":\n";
    s->quizHistory.display();
    s->quizHistory.displayAsChain();
}

void ConsoleMenu::searchHistoryByTopic() {
    Student* s = askForStudent(manager);
    if (s == nullptr) return;
    int topicId = 0;
    if (!askInt("Topic ID", topicId)) return;
    s->quizHistory.displayForTopic(topicId);
}

void ConsoleMenu::deleteQuizAttempt() {
    int studentId = 0, attemptId = 0;
    if (!askInt("Student ID", studentId)) return;
    if (!askInt("Attempt ID to delete", attemptId)) return;
    if (manager.deleteQuizAttempt(studentId, attemptId)) std::cout << "  Attempt deleted.\n";
    else std::cout << "  Error: " << manager.lastError() << "\n";
}

// ------------------------------------------------------ revision planner

void ConsoleMenu::addRevisionTask() {
    int studentId = 0, topicId = 0, priority = 0;
    if (!askInt("Student ID", studentId)) return;
    if (!askInt("Topic ID to revise", topicId)) return;
    if (!askInt("Priority (0-100)", priority)) return;

    int taskId = manager.addRevisionTask(studentId, topicId, priority);
    if (taskId >= 0) std::cout << "  Added as task #" << taskId << ".\n";
    else std::cout << "  Error: " << manager.lastError() << "\n";
}

void ConsoleMenu::showRevisionQueue() {
    Student* s = askForStudent(manager);
    if (s == nullptr) return;
    std::cout << "  Revision queue for " << s->name << ":\n";
    s->revisionQueue.display();
}

void ConsoleMenu::completeNextTask() {
    int studentId = 0;
    if (!askInt("Student ID", studentId)) return;
    RevisionTask done;
    if (manager.completeNextTask(studentId, done)) {
        std::cout << "  Completed task #" << done.taskId << " (topic " << done.topicId << ").\n";
    } else {
        std::cout << "  Error: " << manager.lastError() << "\n";
    }
}

void ConsoleMenu::undoLastAction() {
    int studentId = 0;
    if (!askInt("Student ID", studentId)) return;
    std::string message;
    if (manager.undoLastAction(studentId, message)) std::cout << "  " << message << "\n";
    else std::cout << "  Error: " << manager.lastError() << "\n";
}

// -------------------------------------------------------------- inspector

void ConsoleMenu::showHashTable() {
    manager.students().display();
}

void ConsoleMenu::showUndoStack() {
    Student* s = askForStudent(manager);
    if (s == nullptr) return;
    s->undoStack.display();
}

void ConsoleMenu::showQueueSlots() {
    Student* s = askForStudent(manager);
    if (s == nullptr) return;
    s->revisionQueue.displaySlots();
}

void ConsoleMenu::saveNow() {
    if (storage.save(manager)) std::cout << "  Saved to folder: " << storage.folder() << "\n";
    else std::cout << "  Error: " << storage.lastError() << "\n";
}

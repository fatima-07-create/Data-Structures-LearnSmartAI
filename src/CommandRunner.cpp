// CommandRunner.cpp
#include "CommandRunner.h"
#include "Utils.h"
#include <iostream>

CommandRunner::CommandRunner(StudentManager& studentManager, CsvStorage& csvStorage)
    : manager(studentManager), storage(csvStorage) {}

void CommandRunner::run() {
    std::string line;
    bool keepGoing = true;
    while (keepGoing && std::getline(std::cin, line)) {
        if (trimText(line).empty()) continue;
        std::cout << execute(line, keepGoing) << "\n" << std::flush;
    }
}

std::string CommandRunner::error(const std::string& message) const { return "ERROR|" + message; }
std::string CommandRunner::wrongArguments(const std::string& usage) const { return "ERROR|Wrong arguments. Use: " + usage; }

// Small helper: read part number "index" as an int.
static bool intAt(const std::vector<std::string>& p, std::size_t index, int& number) {
    return index < p.size() && toInt(p[index], number);
}

// Turns 72.456 into "72.5" (no printf needed).
static std::string oneDecimal(double value) {
    int tenths = static_cast<int>(value * 10 + 0.5);
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10);
}

std::string CommandRunner::saveAfterChange(const std::string& okText) {
    if (!storage.save(manager)) return error(storage.lastError());
    return okText;
}

std::string CommandRunner::execute(const std::string& line, bool& keepGoing) {
    Parts p = splitText(trimText(line), '|');
    std::string command = toUpperText(trimText(p[0]));

    if (command == "PING")            return "OK|pong";
    if (command == "ADD_STUDENT")     return addStudent(p);
    if (command == "GET_STUDENT")     return getStudent(p);
    if (command == "REMOVE_STUDENT")  return removeStudent(p);
    if (command == "ADD_ATTEMPT")     return addAttempt(p);
    if (command == "LIST_ATTEMPTS")   return listAttempts(p);
    if (command == "DELETE_ATTEMPT")  return deleteAttempt(p);
    if (command == "ADD_TASK")        return addTask(p);
    if (command == "LIST_TASKS")      return listTasks(p);
    if (command == "COMPLETE_TASK")   return completeTask(p);
    if (command == "UNDO")            return undo(p);
    if (command == "LIST_STUDENTS")   return listStudents(p);
    if (command == "SEARCH_TOPIC")    return searchTopic(p);
    if (command == "HASH_INDEX")      return hashIndex(p);
    if (command == "INSPECT_HASH")    return inspectHash(p);
    if (command == "INSPECT_STACK")   return inspectStack(p);
    if (command == "INSPECT_QUEUE")   return inspectQueue(p);
    if (command == "SAVE")            return saveAfterChange("OK|saved");
    if (command == "EXIT") {
        keepGoing = false;
        return saveAfterChange("OK|bye");
    }
    return error("Unknown command: " + command);
}

std::string CommandRunner::addStudent(const Parts& p) {
    int id = 0;
    if (p.size() != 4 || !toInt(p[1], id)) return wrongArguments("ADD_STUDENT|id|name|email");
    if (!manager.registerStudent(id, p[2], p[3])) return error(manager.lastError());
    return saveAfterChange("OK|Student registered");
}

std::string CommandRunner::getStudent(const Parts& p) {
    int id = 0;
    if (p.size() != 2 || !toInt(p[1], id)) return wrongArguments("GET_STUDENT|id");
    Student* s = manager.findStudent(id);
    if (s == nullptr) return error("Student not found.");
    return "OK|" + std::to_string(s->id) + "|" + s->name + "|" + s->email + "|" +
           std::to_string(s->quizHistory.size()) + "|" +
           std::to_string(s->revisionQueue.size()) + "|" +
           std::to_string(s->undoStack.size()) + "|" +
           oneDecimal(s->quizHistory.overallAveragePercent());
}

std::string CommandRunner::removeStudent(const Parts& p) {
    int id = 0;
    if (p.size() != 2 || !toInt(p[1], id)) return wrongArguments("REMOVE_STUDENT|id");
    if (!manager.removeStudent(id)) return error(manager.lastError());
    return saveAfterChange("OK|Student removed");
}

std::string CommandRunner::addAttempt(const Parts& p) {
    int studentId = 0, topicId = 0, score = 0, total = 0;
    if (p.size() != 6 || !intAt(p, 1, studentId) || !intAt(p, 2, topicId) ||
        !intAt(p, 3, score) || !intAt(p, 4, total)) {
        return wrongArguments("ADD_ATTEMPT|studentId|topicId|score|totalMarks|YYYY-MM-DD");
    }
    int attemptId = manager.addQuizAttempt(studentId, topicId, score, total, trimText(p[5]));
    if (attemptId < 0) return error(manager.lastError());
    return saveAfterChange("OK|" + std::to_string(attemptId));
}

std::string CommandRunner::listAttempts(const Parts& p) {
    int id = 0;
    if (p.size() != 2 || !toInt(p[1], id)) return wrongArguments("LIST_ATTEMPTS|studentId");
    Student* s = manager.findStudent(id);
    if (s == nullptr) return error("Student not found.");

    std::string reply = "OK|" + std::to_string(s->quizHistory.size());
    for (const QuizNode* n = s->quizHistory.head(); n != nullptr; n = n->next) {
        const QuizAttempt& a = n->data;
        reply += "\nROW|" + std::to_string(a.attemptId) + "|" + std::to_string(a.topicId) + "|" +
                 std::to_string(a.score) + "|" + std::to_string(a.totalMarks) + "|" + a.date;
    }
    return reply + "\nEND";
}

std::string CommandRunner::deleteAttempt(const Parts& p) {
    int studentId = 0, attemptId = 0;
    if (p.size() != 3 || !intAt(p, 1, studentId) || !intAt(p, 2, attemptId)) {
        return wrongArguments("DELETE_ATTEMPT|studentId|attemptId");
    }
    if (!manager.deleteQuizAttempt(studentId, attemptId)) return error(manager.lastError());
    return saveAfterChange("OK|Attempt deleted");
}

std::string CommandRunner::addTask(const Parts& p) {
    int studentId = 0, topicId = 0, priority = 0;
    if (p.size() != 4 || !intAt(p, 1, studentId) || !intAt(p, 2, topicId) || !intAt(p, 3, priority)) {
        return wrongArguments("ADD_TASK|studentId|topicId|priority");
    }
    int taskId = manager.addRevisionTask(studentId, topicId, priority);
    if (taskId < 0) return error(manager.lastError());
    return saveAfterChange("OK|" + std::to_string(taskId));
}

std::string CommandRunner::listTasks(const Parts& p) {
    int id = 0;
    if (p.size() != 2 || !toInt(p[1], id)) return wrongArguments("LIST_TASKS|studentId");
    Student* s = manager.findStudent(id);
    if (s == nullptr) return error("Student not found.");

    std::string reply = "OK|" + std::to_string(s->revisionQueue.size());
    for (int i = 0; i < s->revisionQueue.size(); i++) {
        RevisionTask t;
        s->revisionQueue.getAt(i, t);
        reply += "\nROW|" + std::to_string(t.taskId) + "|" + std::to_string(t.topicId) + "|" +
                 t.status + "|" + std::to_string(t.priority);
    }
    return reply + "\nEND";
}

std::string CommandRunner::completeTask(const Parts& p) {
    int id = 0;
    if (p.size() != 2 || !toInt(p[1], id)) return wrongArguments("COMPLETE_TASK|studentId");
    RevisionTask done;
    if (!manager.completeNextTask(id, done)) return error(manager.lastError());
    return saveAfterChange("OK|" + std::to_string(done.taskId) + "|" + std::to_string(done.topicId));
}

std::string CommandRunner::undo(const Parts& p) {
    int id = 0;
    if (p.size() != 2 || !toInt(p[1], id)) return wrongArguments("UNDO|studentId");
    std::string message;
    if (!manager.undoLastAction(id, message)) return error(manager.lastError());
    return saveAfterChange("OK|" + message);
}

// ----------------------------------------------------- read-only commands
// These only LOOK at the data. The Streamlit inspector uses them to draw the structures.

std::string CommandRunner::listStudents(const Parts& p) {
    if (p.size() != 1) return wrongArguments("LIST_STUDENTS");
    const StudentHashTable& table = manager.students();

    std::string rows;
    for (int b = 0; b < table.bucketCount(); b++) {
        for (const HashNode* node = table.bucketHead(b); node != nullptr; node = node->next) {
            const Student* s = node->student;
            rows += "\nROW|" + std::to_string(s->id) + "|" + s->name + "|" + s->email + "|" +
                    std::to_string(s->quizHistory.size()) + "|" + std::to_string(s->revisionQueue.size());
        }
    }
    return "OK|" + std::to_string(table.studentCount()) + rows + "\nEND";
}

std::string CommandRunner::searchTopic(const Parts& p) {
    int studentId = 0, topicId = 0;
    if (p.size() != 3 || !intAt(p, 1, studentId) || !intAt(p, 2, topicId)) {
        return wrongArguments("SEARCH_TOPIC|studentId|topicId");
    }
    Student* s = manager.findStudent(studentId);
    if (s == nullptr) return error("Student not found.");

    std::string rows;
    for (const QuizNode* n = s->quizHistory.head(); n != nullptr; n = n->next) {
        const QuizAttempt& a = n->data;
        if (a.topicId != topicId) continue;
        rows += "\nROW|" + std::to_string(a.attemptId) + "|" + std::to_string(a.topicId) + "|" +
                std::to_string(a.score) + "|" + std::to_string(a.totalMarks) + "|" + a.date;
    }
    return "OK|" + std::to_string(s->quizHistory.countForTopic(topicId)) + rows + "\nEND";
}

// Which bucket would this ID go to? Works for IDs that are not registered yet (good for collision demos).
std::string CommandRunner::hashIndex(const Parts& p) {
    int id = 0;
    if (p.size() != 2 || !toInt(p[1], id)) return wrongArguments("HASH_INDEX|id");
    const StudentHashTable& table = manager.students();
    return "OK|" + std::to_string(table.hashFunction(id)) + "|" + std::to_string(table.bucketCount());
}

std::string CommandRunner::inspectHash(const Parts& p) {
    if (p.size() != 1) return wrongArguments("INSPECT_HASH");
    const StudentHashTable& table = manager.students();

    std::string rows;
    for (int b = 0; b < table.bucketCount(); b++) {
        int position = 0;
        for (const HashNode* node = table.bucketHead(b); node != nullptr; node = node->next) {
            rows += "\nROW|" + std::to_string(b) + "|" + std::to_string(position) + "|" +
                    std::to_string(node->key) + "|" + node->student->name;
            position++;
        }
    }
    return "OK|" + std::to_string(table.bucketCount()) + "|" + std::to_string(table.studentCount()) + "|" +
           std::to_string(table.collisionCount()) + rows + "\nEND";
}

// Top of the stack comes first (position 0).
std::string CommandRunner::inspectStack(const Parts& p) {
    int id = 0;
    if (p.size() != 2 || !toInt(p[1], id)) return wrongArguments("INSPECT_STACK|studentId");
    Student* s = manager.findStudent(id);
    if (s == nullptr) return error("Student not found.");

    std::string rows;
    int size = s->undoStack.size();
    for (int position = 0; position < size; position++) {
        UndoAction action;
        s->undoStack.getAt(size - 1 - position, action);
        std::string type = (action.type == ActionType::AddTask) ? "AddTask" : "CompleteTask";
        rows += "\nROW|" + std::to_string(position) + "|" + type + "|" +
                std::to_string(action.task.taskId) + "|" + std::to_string(action.task.topicId);
    }
    return "OK|" + std::to_string(size) + "|" + std::to_string(MAX_UNDO_ACTIONS) + rows + "\nEND";
}

// Every raw array slot, so the UI can show front / rear / wraparound.
std::string CommandRunner::inspectQueue(const Parts& p) {
    int id = 0;
    if (p.size() != 2 || !toInt(p[1], id)) return wrongArguments("INSPECT_QUEUE|studentId");
    Student* s = manager.findStudent(id);
    if (s == nullptr) return error("Student not found.");

    const RevisionQueue& queue = s->revisionQueue;
    std::string rows;
    for (int slot = 0; slot < MAX_REVISION_TASKS; slot++) {
        RevisionTask t;
        if (queue.readSlot(slot, t)) {
            rows += "\nROW|" + std::to_string(slot) + "|1|" + std::to_string(t.taskId) + "|" +
                    std::to_string(t.topicId) + "|" + std::to_string(t.priority);
        } else {
            rows += "\nROW|" + std::to_string(slot) + "|0|0|0|0";
        }
    }
    return "OK|" + std::to_string(queue.frontSlot()) + "|" + std::to_string(queue.rearSlot()) + "|" +
           std::to_string(queue.size()) + "|" + std::to_string(MAX_REVISION_TASKS) + rows + "\nEND";
}

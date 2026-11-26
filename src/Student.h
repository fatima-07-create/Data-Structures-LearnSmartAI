// Student.h
// One student record. Each student owns their own quiz history, revision queue and undo stack.
#pragma once
#include <string>
#include "QuizHistoryList.h"
#include "RevisionQueue.h"
#include "UndoStack.h"

class Student {
public:
    Student(int studentId, const std::string& studentName, const std::string& studentEmail)
        : id(studentId), name(studentName), email(studentEmail) {}

    int         id;
    std::string name;
    std::string email;

    QuizHistoryList quizHistory;    // Linked List
    RevisionQueue   revisionQueue;  // Circular Queue
    UndoStack       undoStack;      // Stack
};

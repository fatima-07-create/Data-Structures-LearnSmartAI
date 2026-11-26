// Models.h
// Small plain data types that the whole project shares.
#pragma once
#include <string>

// Sizes of the two array-based structures (small on purpose, easy to explain).
const int MAX_REVISION_TASKS = 10;   // circular queue capacity (per student)
const int MAX_UNDO_ACTIONS   = 20;   // stack capacity (per student)

// One saved quiz result. These are stored inside the linked list.
struct QuizAttempt {
    int attemptId  = 0;
    int studentId  = 0;
    int topicId    = 0;
    int score      = 0;
    int totalMarks = 0;
    std::string date;            // format: YYYY-MM-DD

    double percentage() const {
        if (totalMarks <= 0) return 0.0;
        return (score * 100.0) / totalMarks;
    }
};

// One topic the student should revise. These wait inside the circular queue.
struct RevisionTask {
    int taskId   = 0;
    int topicId  = 0;
    int priority = 0;                 // 0 - 100, higher = more urgent
    std::string status = "Pending";   // "Pending" or "Done"
};

// What kind of revision action can be undone.
enum class ActionType { AddTask, CompleteTask };

// One entry on the undo stack.
struct UndoAction {
    ActionType type = ActionType::AddTask;
    RevisionTask task;                // the task that was added or completed
};

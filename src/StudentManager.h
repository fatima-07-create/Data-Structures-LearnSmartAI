// StudentManager.h
// The "brain" of Member A's part. Every student / quiz-history / revision / undo action goes through here,
// so the menu, the command runner and (later) Streamlit all use the same checked code.
#pragma once
#include <string>
#include "StudentHashTable.h"

class StudentManager {
public:
    StudentManager();

    // ----- students (Hash Table) -----
    bool     registerStudent(int studentId, const std::string& name, const std::string& email);
    Student* findStudent(int studentId) const;
    bool     removeStudent(int studentId);

    // ----- quiz history (Linked List) -----
    // Returns the new attempt ID, or -1 if something was wrong (see lastError()).
    int  addQuizAttempt(int studentId, int topicId, int score, int totalMarks, const std::string& date);
    bool deleteQuizAttempt(int studentId, int attemptId);

    // ----- revision planner (Circular Queue + Stack) -----
    // Returns the new task ID, or -1 if something was wrong.
    int  addRevisionTask(int studentId, int topicId, int priority);
    bool completeNextTask(int studentId, RevisionTask& doneTask);
    bool undoLastAction(int studentId, std::string& message);

    // ----- used by CsvStorage while loading files (no undo history is recorded) -----
    bool loadAttempt(const QuizAttempt& attempt);
    bool loadTask(int studentId, const RevisionTask& task);

    const StudentHashTable& students() const;
    const std::string&      lastError() const;

private:
    void setError(const std::string& message);
    void rememberAction(Student& student, const UndoAction& action);

    StudentHashTable table;
    int nextAttemptId;
    int nextTaskId;
    std::string errorText;
};

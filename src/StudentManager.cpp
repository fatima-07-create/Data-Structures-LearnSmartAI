// StudentManager.cpp
#include "StudentManager.h"
#include "Utils.h"

StudentManager::StudentManager() : nextAttemptId(1), nextTaskId(1) {}

void StudentManager::setError(const std::string& message) { errorText = message; }
const std::string& StudentManager::lastError() const { return errorText; }
const StudentHashTable& StudentManager::students() const { return table; }

// ---------------------------------------------------------------- students

bool StudentManager::registerStudent(int studentId, const std::string& name, const std::string& email) {
    if (studentId <= 0) {
        setError("Student ID must be a positive number.");
        return false;
    }
    std::string cleanName = cleanText(name);
    if (cleanName.empty()) {
        setError("Student name cannot be empty.");
        return false;
    }

    Student* student = new Student(studentId, cleanName, cleanText(email));
    if (!table.insert(student)) {
        delete student;                      // the table did not take it, so we clean it up
        setError("A student with ID " + std::to_string(studentId) + " already exists.");
        return false;
    }
    return true;
}

Student* StudentManager::findStudent(int studentId) const {
    return table.search(studentId);
}

bool StudentManager::removeStudent(int studentId) {
    if (!table.remove(studentId)) {
        setError("Student not found.");
        return false;
    }
    return true;
}

// ------------------------------------------------------------ quiz history

int StudentManager::addQuizAttempt(int studentId, int topicId, int score, int totalMarks, const std::string& date) {
    Student* student = table.search(studentId);
    if (student == nullptr)               { setError("Student not found."); return -1; }
    if (topicId <= 0)                     { setError("Topic ID must be a positive number."); return -1; }
    if (totalMarks <= 0)                  { setError("Total marks must be more than 0."); return -1; }
    if (score < 0 || score > totalMarks)  { setError("Score must be between 0 and total marks."); return -1; }
    if (!isValidDate(date))               { setError("Date must look like YYYY-MM-DD."); return -1; }

    QuizAttempt attempt;
    attempt.attemptId  = nextAttemptId;
    attempt.studentId  = studentId;
    attempt.topicId    = topicId;
    attempt.score      = score;
    attempt.totalMarks = totalMarks;
    attempt.date       = date;

    student->quizHistory.insertAtTail(attempt);
    nextAttemptId++;
    return attempt.attemptId;
}

bool StudentManager::deleteQuizAttempt(int studentId, int attemptId) {
    Student* student = table.search(studentId);
    if (student == nullptr) {
        setError("Student not found.");
        return false;
    }
    if (!student->quizHistory.deleteByAttemptId(attemptId)) {
        setError("Attempt not found for this student.");
        return false;
    }
    return true;
}

// -------------------------------------------------------- revision planner

// If the undo stack is full we simply start a fresh history. This keeps undo always correct.
void StudentManager::rememberAction(Student& student, const UndoAction& action) {
    if (student.undoStack.isFull()) student.undoStack.clear();
    student.undoStack.push(action);
}

int StudentManager::addRevisionTask(int studentId, int topicId, int priority) {
    Student* student = table.search(studentId);
    if (student == nullptr)                 { setError("Student not found."); return -1; }
    if (topicId <= 0)                       { setError("Topic ID must be a positive number."); return -1; }
    if (priority < 0 || priority > 100)     { setError("Priority must be between 0 and 100."); return -1; }
    if (student->revisionQueue.contains(topicId)) { setError("This topic is already in the revision queue."); return -1; }
    if (student->revisionQueue.isFull())    { setError("Revision queue is full."); return -1; }

    RevisionTask task;
    task.taskId   = nextTaskId;
    task.topicId  = topicId;
    task.priority = priority;
    task.status   = "Pending";

    student->revisionQueue.enqueue(task);
    nextTaskId++;

    UndoAction action;
    action.type = ActionType::AddTask;
    action.task = task;
    rememberAction(*student, action);
    return task.taskId;
}

bool StudentManager::completeNextTask(int studentId, RevisionTask& doneTask) {
    Student* student = table.search(studentId);
    if (student == nullptr) {
        setError("Student not found.");
        return false;
    }

    RevisionTask task;
    if (!student->revisionQueue.dequeue(task)) {
        setError("There are no pending revision tasks.");
        return false;
    }

    // Save the task as it was (Pending) so undo can put it back unchanged.
    UndoAction action;
    action.type = ActionType::CompleteTask;
    action.task = task;
    rememberAction(*student, action);

    task.status = "Done";
    doneTask = task;
    return true;
}

bool StudentManager::undoLastAction(int studentId, std::string& message) {
    Student* student = table.search(studentId);
    if (student == nullptr) {
        setError("Student not found.");
        return false;
    }

    UndoAction action;
    if (!student->undoStack.peek(action)) {
        setError("Nothing to undo.");
        return false;
    }

    if (action.type == ActionType::AddTask) {
        // Undo "add": take that task back off the rear of the queue.
        RevisionTask removed;
        if (!student->revisionQueue.removeFromRear(removed) || removed.taskId != action.task.taskId) {
            setError("Could not undo: the queue no longer matches the last action.");
            return false;
        }
    } else {
        // Undo "complete": put the task back at the front of the queue.
        if (!student->revisionQueue.insertAtFront(action.task)) {
            setError("Could not undo: the revision queue is full.");
            return false;
        }
    }

    student->undoStack.pop(action);
    message = "Undone: " + describeAction(action);
    return true;
}

// ------------------------------------------------------ loading from files

bool StudentManager::loadAttempt(const QuizAttempt& attempt) {
    Student* student = table.search(attempt.studentId);
    if (student == nullptr || attempt.attemptId <= 0) return false;
    if (attempt.totalMarks <= 0 || attempt.score < 0 || attempt.score > attempt.totalMarks) return false;
    if (attempt.topicId <= 0 || !isValidDate(attempt.date)) return false;
    if (student->quizHistory.searchByAttemptId(attempt.attemptId) != nullptr) return false;

    student->quizHistory.insertAtTail(attempt);
    if (attempt.attemptId >= nextAttemptId) nextAttemptId = attempt.attemptId + 1;
    return true;
}

bool StudentManager::loadTask(int studentId, const RevisionTask& task) {
    Student* student = table.search(studentId);
    if (student == nullptr || task.taskId <= 0 || task.topicId <= 0) return false;
    if (task.priority < 0 || task.priority > 100) return false;
    if (student->revisionQueue.contains(task.topicId)) return false;

    if (!student->revisionQueue.enqueue(task)) return false;
    if (task.taskId >= nextTaskId) nextTaskId = task.taskId + 1;
    return true;
}

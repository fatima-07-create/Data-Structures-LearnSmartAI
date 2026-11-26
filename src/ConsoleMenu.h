// ConsoleMenu.h
// A simple text menu so Member A's whole part can be tested and demonstrated without Streamlit.
#pragma once
#include "CsvStorage.h"
#include "StudentManager.h"

class ConsoleMenu {
public:
    ConsoleMenu(StudentManager& studentManager, CsvStorage& csvStorage);
    void run();

private:
    void printMenu() const;

    void registerStudent();
    void findStudent();
    void removeStudent();
    void addQuizAttempt();
    void showQuizHistory();
    void searchHistoryByTopic();
    void deleteQuizAttempt();
    void addRevisionTask();
    void showRevisionQueue();
    void completeNextTask();
    void undoLastAction();
    void showHashTable();
    void showUndoStack();
    void showQueueSlots();
    void saveNow();

    StudentManager& manager;
    CsvStorage&     storage;
};

// CommandRunner.h
// "Engine mode": reads one command per line from the keyboard / a pipe and prints one response.
// This is what the Python bridge will talk to. The commands are listed in docs/PROTOCOL.md.
#pragma once
#include <string>
#include <vector>
#include "CsvStorage.h"
#include "StudentManager.h"

class CommandRunner {
public:
    CommandRunner(StudentManager& studentManager, CsvStorage& csvStorage);
    void run();

    // Runs one command line and returns the reply text (also used by the self tests).
    std::string execute(const std::string& line, bool& keepGoing);

private:
    typedef std::vector<std::string> Parts;

    std::string addStudent(const Parts& p);
    std::string getStudent(const Parts& p);
    std::string removeStudent(const Parts& p);
    std::string addAttempt(const Parts& p);
    std::string listAttempts(const Parts& p);
    std::string deleteAttempt(const Parts& p);
    std::string addTask(const Parts& p);
    std::string listTasks(const Parts& p);
    std::string completeTask(const Parts& p);
    std::string undo(const Parts& p);
    std::string listStudents(const Parts& p);
    std::string searchTopic(const Parts& p);
    std::string hashIndex(const Parts& p);
    std::string inspectHash(const Parts& p);
    std::string inspectStack(const Parts& p);
    std::string inspectQueue(const Parts& p);

    std::string saveAfterChange(const std::string& okText);   // saves CSV, then returns okText
    std::string error(const std::string& message) const;
    std::string wrongArguments(const std::string& usage) const;

    StudentManager& manager;
    CsvStorage&     storage;
};

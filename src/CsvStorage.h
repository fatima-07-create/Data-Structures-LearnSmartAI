// CsvStorage.h
// Saves and loads Member A's data as simple CSV files:
//   students.csv, quiz_results.csv, revision_tasks.csv
#pragma once
#include <string>
#include <vector>
#include "StudentManager.h"

class CsvStorage {
public:
    explicit CsvStorage(const std::string& dataFolder);

    bool load(StudentManager& manager);          // missing files are fine (empty start)
    bool save(const StudentManager& manager);    // false if a file could not be written

    int  skippedRows() const;                    // bad rows ignored during the last load
    const std::string& lastError() const;
    const std::string& folder() const;

    // Looks for a "data" folder in the usual places (works from Visual Studio and from a terminal).
    static std::string findDataFolder();

private:
    typedef std::vector<std::vector<std::string> > CsvRows;

    bool readRows(const std::string& fileName, CsvRows& rows) const;
    void loadStudents(StudentManager& manager);
    void loadAttempts(StudentManager& manager);
    void loadTasks(StudentManager& manager);

    std::string dataFolder;
    std::string errorText;
    int skipped;
};

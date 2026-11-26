// CsvStorage.cpp
#include "CsvStorage.h"
#include "Utils.h"
#include <filesystem>
#include <fstream>

CsvStorage::CsvStorage(const std::string& folderPath) : dataFolder(folderPath), skipped(0) {}

int CsvStorage::skippedRows() const { return skipped; }
const std::string& CsvStorage::lastError() const { return errorText; }
const std::string& CsvStorage::folder() const { return dataFolder; }

std::string CsvStorage::findDataFolder() {
    const char* candidates[] = { "data", "../data", "../../data", "../../../data" };
    std::error_code ec;
    for (const char* path : candidates) {
        if (std::filesystem::exists(std::string(path) + "/students.csv", ec)) return path;
    }
    return "data";    // not found: use (and later create) a "data" folder here
}

// Reads a CSV file into rows of text. The first line is the header, so we skip it.
bool CsvStorage::readRows(const std::string& fileName, CsvRows& rows) const {
    std::ifstream file(dataFolder + "/" + fileName);
    if (!file.is_open()) return false;

    std::string line;
    bool isHeader = true;
    while (std::getline(file, line)) {
        line = trimText(line);
        if (isHeader) { isHeader = false; continue; }
        if (line.empty()) continue;
        rows.push_back(splitText(line, ','));
    }
    return true;
}

// ----------------------------------------------------------------- loading

bool CsvStorage::load(StudentManager& manager) {
    skipped = 0;
    errorText.clear();
    // Order matters: students first, because attempts and tasks belong to a student.
    loadStudents(manager);
    loadAttempts(manager);
    loadTasks(manager);
    return true;
}

void CsvStorage::loadStudents(StudentManager& manager) {
    CsvRows rows;
    if (!readRows("students.csv", rows)) return;

    for (const auto& row : rows) {
        int id = 0;
        if (row.size() != 3 || !toInt(row[0], id) || !manager.registerStudent(id, row[1], row[2])) {
            skipped++;
        }
    }
}

void CsvStorage::loadAttempts(StudentManager& manager) {
    CsvRows rows;
    if (!readRows("quiz_results.csv", rows)) return;

    for (const auto& row : rows) {
        QuizAttempt a;
        bool ok = row.size() == 6 &&
                  toInt(row[0], a.attemptId) && toInt(row[1], a.studentId) &&
                  toInt(row[2], a.topicId)   && toInt(row[3], a.score) &&
                  toInt(row[4], a.totalMarks);
        if (ok) a.date = trimText(row[5]);
        if (!ok || !manager.loadAttempt(a)) skipped++;
    }
}

void CsvStorage::loadTasks(StudentManager& manager) {
    CsvRows rows;
    if (!readRows("revision_tasks.csv", rows)) return;

    for (const auto& row : rows) {
        RevisionTask t;
        int studentId = 0;
        bool ok = row.size() == 5 &&
                  toInt(row[0], t.taskId) && toInt(row[1], studentId) &&
                  toInt(row[2], t.topicId) && toInt(row[4], t.priority);
        if (!ok) { skipped++; continue; }

        t.status = trimText(row[3]);
        if (t.status != "Pending") continue;      // only pending tasks live in the queue
        if (!manager.loadTask(studentId, t)) skipped++;
    }
}

// ------------------------------------------------------------------ saving

bool CsvStorage::save(const StudentManager& manager) {
    errorText.clear();
    std::error_code ec;
    std::filesystem::create_directories(dataFolder, ec);

    std::ofstream studentsFile(dataFolder + "/students.csv");
    std::ofstream attemptsFile(dataFolder + "/quiz_results.csv");
    std::ofstream tasksFile(dataFolder + "/revision_tasks.csv");
    if (!studentsFile.is_open() || !attemptsFile.is_open() || !tasksFile.is_open()) {
        errorText = "Could not write the CSV files in folder: " + dataFolder;
        return false;
    }

    studentsFile << "student_id,name,email\n";
    attemptsFile << "attempt_id,student_id,topic_id,score,total_marks,date\n";
    tasksFile    << "task_id,student_id,topic_id,status,priority\n";

    // Walk every bucket and every student inside it.
    const StudentHashTable& table = manager.students();
    for (int b = 0; b < table.bucketCount(); b++) {
        for (const HashNode* node = table.bucketHead(b); node != nullptr; node = node->next) {
            const Student* s = node->student;
            studentsFile << s->id << "," << s->name << "," << s->email << "\n";

            for (const QuizNode* q = s->quizHistory.head(); q != nullptr; q = q->next) {
                const QuizAttempt& a = q->data;
                attemptsFile << a.attemptId << "," << s->id << "," << a.topicId << ","
                             << a.score << "," << a.totalMarks << "," << a.date << "\n";
            }

            for (int i = 0; i < s->revisionQueue.size(); i++) {
                RevisionTask t;
                s->revisionQueue.getAt(i, t);
                tasksFile << t.taskId << "," << s->id << "," << t.topicId << ","
                          << t.status << "," << t.priority << "\n";
            }
        }
    }

    if (!studentsFile.good() || !attemptsFile.good() || !tasksFile.good()) {
        errorText = "Writing to the CSV files failed.";
        return false;
    }
    return true;
}

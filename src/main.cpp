// main.cpp
// LearnSmart - Member A part (Hash Table, Linked List, Stack, Circular Queue + CSV storage).
//
// How to start it:
//   LearnSmart            -> text menu
//   LearnSmart --engine   -> command mode (used later by the Python bridge)
//   LearnSmart --test     -> run the self tests and exit
//   add  --data <folder>  -> use a different data folder
#include <iostream>
#include <string>
#include "CommandRunner.h"
#include "ConsoleMenu.h"
#include "CsvStorage.h"
#include "SelfTests.h"
#include "StudentManager.h"

int main(int argc, char* argv[]) {
    std::string mode;
    std::string dataFolder;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--engine" || arg == "--test") mode = arg;
        else if (arg == "--data" && i + 1 < argc) dataFolder = argv[++i];
    }

    if (mode == "--test") {
        return runSelfTests() == 0 ? 0 : 1;
    }

    if (dataFolder.empty()) dataFolder = CsvStorage::findDataFolder();

    StudentManager manager;
    CsvStorage storage(dataFolder);
    storage.load(manager);

    if (mode == "--engine") {
        if (storage.skippedRows() > 0) {
            std::cerr << "Warning: " << storage.skippedRows() << " bad CSV row(s) were skipped.\n";
        }
        CommandRunner runner(manager, storage);
        runner.run();
        return 0;
    }

    std::cout << "Data folder: " << storage.folder() << "\n"
              << "Students loaded: " << manager.students().studentCount() << "\n";
    if (storage.skippedRows() > 0) {
        std::cout << "Warning: " << storage.skippedRows() << " bad CSV row(s) were skipped.\n";
    }
    ConsoleMenu menu(manager, storage);
    menu.run();
    return 0;
}

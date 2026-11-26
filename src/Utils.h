// Utils.h
// Tiny text helpers used by the CSV code, the menu and the command runner.
#pragma once
#include <climits>
#include <cstdlib>
#include <string>
#include <vector>

// Remove spaces / tabs / newlines from both ends of a string.
inline std::string trimText(const std::string& text) {
    const char* spaces = " \t\r\n";
    std::size_t first = text.find_first_not_of(spaces);
    if (first == std::string::npos) return "";
    std::size_t last = text.find_last_not_of(spaces);
    return text.substr(first, last - first + 1);
}

// Cut a line into pieces, e.g. "a,b,c" with ',' -> {"a", "b", "c"}.
inline std::vector<std::string> splitText(const std::string& text, char separator) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : text) {
        if (c == separator) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);
    return parts;
}

// Safely turn text into an int. Returns false if the text is not a whole number.
inline bool toInt(const std::string& text, int& number) {
    std::string clean = trimText(text);
    if (clean.empty()) return false;
    char* end = nullptr;
    long long value = std::strtoll(clean.c_str(), &end, 10);
    if (*end != '\0') return false;
    if (value > INT_MAX || value < INT_MIN) return false;
    number = static_cast<int>(value);
    return true;
}

// Commas and pipes would break our CSV / command formats, so swap them for spaces.
inline std::string cleanText(const std::string& text) {
    std::string result = text;
    for (char& c : result) {
        if (c == ',' || c == '|' || c == '\n' || c == '\r') c = ' ';
    }
    return trimText(result);
}

// Checks the simple date shape YYYY-MM-DD (with a sensible month and day).
inline bool isValidDate(const std::string& date) {
    if (date.size() != 10) return false;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            if (date[i] != '-') return false;
        } else if (date[i] < '0' || date[i] > '9') {
            return false;
        }
    }
    int month = (date[5] - '0') * 10 + (date[6] - '0');
    int day   = (date[8] - '0') * 10 + (date[9] - '0');
    return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

// Makes a command word upper case so "ping" and "PING" both work.
inline std::string toUpperText(const std::string& text) {
    std::string result = text;
    for (char& c : result) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    return result;
}

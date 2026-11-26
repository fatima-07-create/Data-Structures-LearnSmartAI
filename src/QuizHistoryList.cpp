// QuizHistoryList.cpp
#include "QuizHistoryList.h"
#include <iostream>

QuizHistoryList::QuizHistoryList() : headNode(nullptr), tailNode(nullptr), count(0) {}

QuizHistoryList::~QuizHistoryList() {
    // Walk the list and free every node.
    while (headNode != nullptr) {
        QuizNode* oldNode = headNode;
        headNode = headNode->next;
        delete oldNode;
    }
}

// Add a new attempt at the end. The tail pointer makes this fast.
void QuizHistoryList::insertAtTail(const QuizAttempt& attempt) {
    QuizNode* node = new QuizNode{attempt, nullptr};
    if (headNode == nullptr) {
        headNode = node;
        tailNode = node;
    } else {
        tailNode->next = node;
        tailNode = node;
    }
    count++;
}

const QuizAttempt* QuizHistoryList::searchByAttemptId(int attemptId) const {
    for (const QuizNode* node = headNode; node != nullptr; node = node->next) {
        if (node->data.attemptId == attemptId) return &node->data;
    }
    return nullptr;
}

// The latest attempt is the LAST matching node, so we keep walking.
const QuizAttempt* QuizHistoryList::findLatestForTopic(int topicId) const {
    const QuizAttempt* latest = nullptr;
    for (const QuizNode* node = headNode; node != nullptr; node = node->next) {
        if (node->data.topicId == topicId) latest = &node->data;
    }
    return latest;
}

int QuizHistoryList::countForTopic(int topicId) const {
    int total = 0;
    for (const QuizNode* node = headNode; node != nullptr; node = node->next) {
        if (node->data.topicId == topicId) total++;
    }
    return total;
}

// Three cases: delete the head, delete a middle/tail node, or not found.
bool QuizHistoryList::deleteByAttemptId(int attemptId) {
    if (headNode == nullptr) return false;

    // Case 1: the first node is the one to delete.
    if (headNode->data.attemptId == attemptId) {
        QuizNode* oldNode = headNode;
        headNode = headNode->next;
        if (headNode == nullptr) tailNode = nullptr;   // list became empty
        delete oldNode;
        count--;
        return true;
    }

    // Case 2: find the node just BEFORE the one we want to delete.
    QuizNode* before = headNode;
    while (before->next != nullptr && before->next->data.attemptId != attemptId) {
        before = before->next;
    }
    if (before->next == nullptr) return false;         // not found

    QuizNode* oldNode = before->next;
    before->next = oldNode->next;
    if (oldNode == tailNode) tailNode = before;        // we removed the last node
    delete oldNode;
    count--;
    return true;
}

int QuizHistoryList::size() const { return count; }
bool QuizHistoryList::isEmpty() const { return count == 0; }
const QuizNode* QuizHistoryList::head() const { return headNode; }

double QuizHistoryList::overallAveragePercent() const {
    if (count == 0) return 0.0;
    double sum = 0.0;
    for (const QuizNode* node = headNode; node != nullptr; node = node->next) {
        sum += node->data.percentage();
    }
    return sum / count;
}

static void printAttempt(const QuizAttempt& a) {
    std::cout << "  Attempt #" << a.attemptId
              << " | Topic " << a.topicId
              << " | " << a.score << "/" << a.totalMarks
              << " (" << static_cast<int>(a.percentage() + 0.5) << "%)"
              << " | " << a.date << "\n";
}

void QuizHistoryList::display() const {
    if (headNode == nullptr) {
        std::cout << "  (no quiz attempts yet)\n";
        return;
    }
    for (const QuizNode* node = headNode; node != nullptr; node = node->next) {
        printAttempt(node->data);
    }
}

void QuizHistoryList::displayForTopic(int topicId) const {
    bool found = false;
    for (const QuizNode* node = headNode; node != nullptr; node = node->next) {
        if (node->data.topicId == topicId) {
            printAttempt(node->data);
            found = true;
        }
    }
    if (!found) std::cout << "  (no attempts for topic " << topicId << ")\n";
}

void QuizHistoryList::displayAsChain() const {
    std::cout << "  HEAD -> ";
    for (const QuizNode* node = headNode; node != nullptr; node = node->next) {
        std::cout << "[#" << node->data.attemptId << "] -> ";
    }
    std::cout << "NULL\n";
}

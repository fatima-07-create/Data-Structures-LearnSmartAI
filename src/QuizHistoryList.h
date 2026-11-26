// QuizHistoryList.h
// Singly linked list that keeps ONE student's quiz attempts in the order they happened.
#pragma once
#include "Models.h"

struct QuizNode {
    QuizAttempt data;
    QuizNode*   next;
};

class QuizHistoryList {
public:
    QuizHistoryList();
    ~QuizHistoryList();

    // A list owns its nodes, so copying it by accident is not allowed.
    QuizHistoryList(const QuizHistoryList&) = delete;
    QuizHistoryList& operator=(const QuizHistoryList&) = delete;

    void insertAtTail(const QuizAttempt& attempt);
    const QuizAttempt* searchByAttemptId(int attemptId) const;
    const QuizAttempt* findLatestForTopic(int topicId) const;
    int  countForTopic(int topicId) const;
    bool deleteByAttemptId(int attemptId);

    int    size() const;
    bool   isEmpty() const;
    double overallAveragePercent() const;      // 0 when the list is empty
    const QuizNode* head() const;              // lets other code walk the list

    void display() const;
    void displayForTopic(int topicId) const;
    void displayAsChain() const;               // [#1] -> [#2] -> NULL

private:
    QuizNode* headNode;
    QuizNode* tailNode;
    int       count;
};

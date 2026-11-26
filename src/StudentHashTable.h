// StudentHashTable.h
// Hash table with chaining. Finds a student by numeric ID in (almost) one step.
#pragma once
#include "Student.h"

// One link in a bucket's chain.
struct HashNode {
    int        key;        // the student ID
    Student*   student;
    HashNode*  next;
};

class StudentHashTable {
public:
    explicit StudentHashTable(int startBuckets = 11);
    ~StudentHashTable();

    StudentHashTable(const StudentHashTable&) = delete;
    StudentHashTable& operator=(const StudentHashTable&) = delete;

    // The table takes ownership of the student when insert returns true.
    // When it returns false (duplicate ID) the caller still owns the student.
    bool     insert(Student* student);
    Student* search(int studentId) const;
    bool     remove(int studentId);        // also deletes the student

    int hashFunction(int studentId) const; // which bucket an ID goes to
    int studentCount() const;
    int bucketCount() const;
    int collisionCount() const;            // students sharing a bucket with an earlier one
    const HashNode* bucketHead(int bucketIndex) const;

    void display() const;

private:
    void rehash();                         // grow the table and re-place every student

    HashNode** buckets;
    int bucketTotal;
    int count;
};

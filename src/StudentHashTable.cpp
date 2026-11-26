// StudentHashTable.cpp
#include "StudentHashTable.h"
#include <iostream>

StudentHashTable::StudentHashTable(int startBuckets) {
    if (startBuckets < 3) startBuckets = 3;
    bucketTotal = startBuckets;
    count = 0;
    buckets = new HashNode*[bucketTotal];
    for (int i = 0; i < bucketTotal; i++) buckets[i] = nullptr;
}

StudentHashTable::~StudentHashTable() {
    for (int i = 0; i < bucketTotal; i++) {
        HashNode* node = buckets[i];
        while (node != nullptr) {
            HashNode* oldNode = node;
            node = node->next;
            delete oldNode->student;
            delete oldNode;
        }
    }
    delete[] buckets;
}

// Simple and easy to explain: the remainder of ID divided by the bucket count.
int StudentHashTable::hashFunction(int studentId) const {
    int index = studentId % bucketTotal;
    if (index < 0) index += bucketTotal;
    return index;
}

bool StudentHashTable::insert(Student* student) {
    if (student == nullptr) return false;
    if (search(student->id) != nullptr) return false;      // duplicate ID

    // Too crowded? (more than 75% full) -> grow first.
    if ((count + 1) * 4 > bucketTotal * 3) rehash();

    int index = hashFunction(student->id);
    HashNode* node = new HashNode{student->id, student, buckets[index]};
    buckets[index] = node;                                  // add at the front of the chain
    count++;
    return true;
}

Student* StudentHashTable::search(int studentId) const {
    for (HashNode* node = buckets[hashFunction(studentId)]; node != nullptr; node = node->next) {
        if (node->key == studentId) return node->student;
    }
    return nullptr;
}

bool StudentHashTable::remove(int studentId) {
    int index = hashFunction(studentId);
    HashNode* before = nullptr;
    HashNode* node = buckets[index];

    while (node != nullptr && node->key != studentId) {
        before = node;
        node = node->next;
    }
    if (node == nullptr) return false;                      // not found

    if (before == nullptr) buckets[index] = node->next;     // it was the first in the chain
    else                   before->next = node->next;

    delete node->student;
    delete node;
    count--;
    return true;
}

// Make the table bigger (about double) and move every node to its new bucket.
// We re-use the same nodes, so no student is copied or lost.
void StudentHashTable::rehash() {
    int oldTotal = bucketTotal;
    HashNode** oldBuckets = buckets;

    bucketTotal = oldTotal * 2 + 1;
    buckets = new HashNode*[bucketTotal];
    for (int i = 0; i < bucketTotal; i++) buckets[i] = nullptr;

    for (int i = 0; i < oldTotal; i++) {
        HashNode* node = oldBuckets[i];
        while (node != nullptr) {
            HashNode* nextNode = node->next;
            int newIndex = hashFunction(node->key);         // uses the NEW bucketTotal
            node->next = buckets[newIndex];
            buckets[newIndex] = node;
            node = nextNode;
        }
    }
    delete[] oldBuckets;
}

int StudentHashTable::studentCount() const { return count; }
int StudentHashTable::bucketCount() const { return bucketTotal; }

const HashNode* StudentHashTable::bucketHead(int bucketIndex) const {
    if (bucketIndex < 0 || bucketIndex >= bucketTotal) return nullptr;
    return buckets[bucketIndex];
}

int StudentHashTable::collisionCount() const {
    int collisions = 0;
    for (int i = 0; i < bucketTotal; i++) {
        int inBucket = 0;
        for (HashNode* node = buckets[i]; node != nullptr; node = node->next) inBucket++;
        if (inBucket > 1) collisions += inBucket - 1;
    }
    return collisions;
}

void StudentHashTable::display() const {
    std::cout << "  Buckets: " << bucketTotal << ", students: " << count
              << ", collisions: " << collisionCount() << "\n";
    for (int i = 0; i < bucketTotal; i++) {
        std::cout << "  Bucket " << i << ": ";
        for (HashNode* node = buckets[i]; node != nullptr; node = node->next) {
            std::cout << "[" << node->key << " " << node->student->name << "] -> ";
        }
        std::cout << "NULL\n";
    }
}

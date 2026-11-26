// RevisionQueue.cpp
#include "RevisionQueue.h"
#include <iostream>

// rearIndex starts at the last slot so the first enqueue lands in slot 0.
RevisionQueue::RevisionQueue() : frontIndex(0), rearIndex(MAX_REVISION_TASKS - 1), count(0) {}

bool RevisionQueue::isEmpty() const { return count == 0; }
bool RevisionQueue::isFull() const { return count == MAX_REVISION_TASKS; }
int  RevisionQueue::size() const { return count; }

bool RevisionQueue::enqueue(const RevisionTask& task) {
    if (isFull()) return false;
    rearIndex = (rearIndex + 1) % MAX_REVISION_TASKS;   // wrap around with %
    tasks[rearIndex] = task;
    count++;
    return true;
}

bool RevisionQueue::dequeue(RevisionTask& task) {
    if (isEmpty()) return false;
    task = tasks[frontIndex];
    frontIndex = (frontIndex + 1) % MAX_REVISION_TASKS;
    count--;
    return true;
}

bool RevisionQueue::front(RevisionTask& task) const {
    if (isEmpty()) return false;
    task = tasks[frontIndex];
    return true;
}

bool RevisionQueue::insertAtFront(const RevisionTask& task) {
    if (isFull()) return false;
    frontIndex = (frontIndex - 1 + MAX_REVISION_TASKS) % MAX_REVISION_TASKS;
    tasks[frontIndex] = task;
    count++;
    return true;
}

bool RevisionQueue::removeFromRear(RevisionTask& task) {
    if (isEmpty()) return false;
    task = tasks[rearIndex];
    rearIndex = (rearIndex - 1 + MAX_REVISION_TASKS) % MAX_REVISION_TASKS;
    count--;
    return true;
}

bool RevisionQueue::getAt(int position, RevisionTask& task) const {
    if (position < 0 || position >= count) return false;
    task = tasks[(frontIndex + position) % MAX_REVISION_TASKS];
    return true;
}

int RevisionQueue::frontSlot() const { return frontIndex; }
int RevisionQueue::rearSlot() const { return rearIndex; }

bool RevisionQueue::readSlot(int slot, RevisionTask& task) const {
    if (slot < 0 || slot >= MAX_REVISION_TASKS) return false;
    int distanceFromFront = (slot - frontIndex + MAX_REVISION_TASKS) % MAX_REVISION_TASKS;
    if (distanceFromFront >= count) return false;           // empty slot
    task = tasks[slot];
    return true;
}

bool RevisionQueue::contains(int topicId) const {
    for (int i = 0; i < count; i++) {
        if (tasks[(frontIndex + i) % MAX_REVISION_TASKS].topicId == topicId) return true;
    }
    return false;
}

void RevisionQueue::display() const {
    if (isEmpty()) {
        std::cout << "  (no pending revision tasks)\n";
        return;
    }
    for (int i = 0; i < count; i++) {
        const RevisionTask& t = tasks[(frontIndex + i) % MAX_REVISION_TASKS];
        std::cout << "  " << (i + 1) << ". Task #" << t.taskId
                  << " | Topic " << t.topicId
                  << " | Priority " << t.priority
                  << " | " << t.status;
        if (i == 0) std::cout << "   <- FRONT (do this first)";
        std::cout << "\n";
    }
}

void RevisionQueue::displaySlots() const {
    std::cout << "  front = " << frontIndex << ", rear = " << rearIndex
              << ", count = " << count << " / " << MAX_REVISION_TASKS << "\n";
    for (int slot = 0; slot < MAX_REVISION_TASKS; slot++) {
        // A slot is "in use" if it sits between front and rear (wrapping around).
        int distanceFromFront = (slot - frontIndex + MAX_REVISION_TASKS) % MAX_REVISION_TASKS;
        bool inUse = distanceFromFront < count;

        std::cout << "  slot " << slot << ": ";
        if (inUse) std::cout << "Task #" << tasks[slot].taskId << " (topic " << tasks[slot].topicId << ")";
        else       std::cout << "-";
        if (inUse && slot == frontIndex) std::cout << "   <- front";
        if (inUse && slot == rearIndex)  std::cout << "   <- rear";
        std::cout << "\n";
    }
}

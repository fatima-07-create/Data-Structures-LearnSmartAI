// RevisionQueue.h
// Circular queue (FIFO) built on an array. Holds the revision tasks a student must do, in order.
#pragma once
#include "Models.h"

class RevisionQueue {
public:
    RevisionQueue();

    bool enqueue(const RevisionTask& task);   // add at the rear
    bool dequeue(RevisionTask& task);         // take from the front
    bool front(RevisionTask& task) const;     // look at the front
    bool isEmpty() const;
    bool isFull() const;
    int  size() const;

    // Two extra moves that only the "undo" feature uses.
    bool insertAtFront(const RevisionTask& task);   // undo a completed task
    bool removeFromRear(RevisionTask& task);        // undo an added task

    bool contains(int topicId) const;                       // is this topic already waiting?
    bool getAt(int position, RevisionTask& task) const;     // 0 = front, 1 = next, ...

    // Used by the inspector to draw the raw array.
    int  frontSlot() const;
    int  rearSlot() const;
    bool readSlot(int slot, RevisionTask& task) const;      // true only if the slot is in use

    void display() const;         // tasks in order
    void displaySlots() const;    // raw array slots, shows front / rear / wraparound

private:
    RevisionTask tasks[MAX_REVISION_TASKS];
    int frontIndex;   // slot of the first task
    int rearIndex;    // slot of the last task
    int count;        // how many tasks are waiting
};

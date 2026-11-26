// UndoStack.h
// Array-based stack (LIFO). It remembers revision actions so the latest one can be undone.
#pragma once
#include <string>
#include "Models.h"

class UndoStack {
public:
    UndoStack();

    bool push(const UndoAction& action);   // false when the stack is full
    bool pop(UndoAction& action);          // false when the stack is empty
    bool peek(UndoAction& action) const;   // look at the top without removing it
    bool isEmpty() const;
    bool isFull() const;
    int  size() const;
    bool getAt(int position, UndoAction& action) const;   // 0 = bottom of the stack
    void clear();
    void display() const;                  // shows the top first

private:
    UndoAction items[MAX_UNDO_ACTIONS];
    int topIndex;                          // -1 means the stack is empty
};

// Short sentence for an action, e.g. "Added task #3 (topic 7)".
std::string describeAction(const UndoAction& action);

// UndoStack.cpp
#include "UndoStack.h"
#include <iostream>

UndoStack::UndoStack() : topIndex(-1) {}

bool UndoStack::isEmpty() const { return topIndex == -1; }
bool UndoStack::isFull() const { return topIndex == MAX_UNDO_ACTIONS - 1; }
int  UndoStack::size() const { return topIndex + 1; }
void UndoStack::clear() { topIndex = -1; }

bool UndoStack::getAt(int position, UndoAction& action) const {
    if (position < 0 || position > topIndex) return false;
    action = items[position];
    return true;
}

bool UndoStack::push(const UndoAction& action) {
    if (isFull()) return false;
    topIndex++;
    items[topIndex] = action;
    return true;
}

bool UndoStack::pop(UndoAction& action) {
    if (isEmpty()) return false;
    action = items[topIndex];
    topIndex--;
    return true;
}

bool UndoStack::peek(UndoAction& action) const {
    if (isEmpty()) return false;
    action = items[topIndex];
    return true;
}

void UndoStack::display() const {
    if (isEmpty()) {
        std::cout << "  (undo stack is empty)\n";
        return;
    }
    for (int i = topIndex; i >= 0; i--) {
        std::cout << "  [" << i << "] " << describeAction(items[i]);
        if (i == topIndex) std::cout << "   <- TOP";
        std::cout << "\n";
    }
}

std::string describeAction(const UndoAction& action) {
    std::string verb = (action.type == ActionType::AddTask) ? "Added" : "Completed";
    return verb + " task #" + std::to_string(action.task.taskId) +
           " (topic " + std::to_string(action.task.topicId) + ")";
}

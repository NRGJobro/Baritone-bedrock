#pragma once

#include "Matrix.h"

class MatrixStack {
public:
    std::stack<Matrix> stack;

private:
    char pad[0x10];

public:
    bool isDirty;

    void push() {
        this->isDirty = true;

        this->stack.push(this->stack.top());
    }

    void pop() {
        this->isDirty = true;
        this->stack.pop();
    }

    Matrix& top() {
        return this->stack.top();
    }
};

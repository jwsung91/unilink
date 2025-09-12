#pragma once

#include <iostream>

class SimpleClass {
   public:
    SimpleClass() {}
    ~SimpleClass() {}
    void print() { std::cout << "Hello from SimpleClass" << std::endl; }
};
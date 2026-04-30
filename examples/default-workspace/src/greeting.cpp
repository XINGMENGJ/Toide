#include "example/msg.h"

#include <iostream>

namespace example {

std::string build_greeting()
{
    return std::string("Hello from Toide example (include/ + src/, multi-file).");
}

void print_greeting()
{
    std::cout << build_greeting() << '\n';
}

} // namespace example

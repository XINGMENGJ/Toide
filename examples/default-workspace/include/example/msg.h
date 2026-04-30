#pragma once

#include <string>

namespace example {

// 声明放在头文件，实现在 src/greeting.cpp（供「编译」页连头文件一起语法检查）
std::string build_greeting();
void print_greeting();

} // namespace example

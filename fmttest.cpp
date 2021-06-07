#include <iostream>
#include <string>
#include "fmt2.hpp"

using namespace fmt::fmt_literal;


int main() {
    const char* s_fmt = "a c-string";
    std::string s_cout = "a std::string";

    std::cout << "{{testing}} {1} and {0} speed\n"_f(s_fmt, s_cout);

    // Positional:
    std::cout << "your {1} goes {0}"_f("here", "args");
    std::cout << "let {0} = {{{1}}}"_f("x", "string");
    //or 
    std::cout << "{0} is {2} better"_f("this", "actually", "much");


}

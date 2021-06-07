# FMT
## Enables Compile-Time *Checked* Formatting 

## Compiler Support
Needs the string literal operator templates [-Wgnu-string-literal-operator-template]
and at least C++14 compliant compiler. 
Supported: 
- [x] GCC
- [x] Clang 

## Syntax
```C++
// Positional:
auto formatted = "your {1} goes {0}"_f("here", "args");
auto fmt2 = "let {0} = {{ "{1}" }}"_f("x", "string");
//or 
std::cout << "{0} is {2} better"_f("this", "actually", "much");

// Sequential: [to be supported soon, ~since is an easier version, but need to optimize~]


```

Syntax is checked at compile-time. 
Missed a brace? Won't compile. 
Trying to output an out-of-bounds argument? Won't compile with a nice (well... kinda, if you think the static_assert is nice, that is) error message. 
[In the next version]: Wrong format specifiers are also compile-time checked.


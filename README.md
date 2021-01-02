# diagctx, providing context to diagnostics

**diagctx** is a C and C++ library which allows to give additional context for diagnostics, including error handling.
The library is licensed under the Boost Sofware License 1.0 and is composed of only two files: `diagctx.h` and `diagctx.c`.
The API and documentation is inside `diagctx.h`.

The library is written in the common C/C++ language, compatible from C89 to latest C++.
It does not perform memory allocations so it is also suitable for embedded systems.
The easiest way to use **diagctx** to your project is to directly include the files in your project.

## Why?

When programming, errors occur in deeply nested functions (memory allocations, user input parsing, etc).
But these functions do not know where their inputs came from, because the context is scattered among parent functions.
The aim of **diagctx** is to provide a utility to retrieve context when diagnostic is needed.

It works well with distant error handling, such as C `longjmp` and C++ exceptions.
When a jump is done from function `func_10` to `func_5` (either because of `longjmp()` or `throw/catch`),
the context acquired across `func_6`...`func_10` is still available for reading.
So `setjmp` and `catch-block` needs only to be positioned where the error handling is actually done.

The alternative idiom in C++ is to catch exceptions, and rethrow them with additional context.
See below for comparison of these alternatives. 

## How does it work

In **diagctx**, a thread_local stack (the data structure) of messages is used.
Messages are pushed when context is available with `diagctx_push`.
Messages are popped when context is obsolete with `diagctx_pop`.
At any point, pushed messages can be read with `diagctx_get`.
`diagctx_get` will also handle messages which are obsolete but were not removed, 
because some `diagctx_pop` were not executed due to a `longjmp()` or `catch-throw`.

## Example

There are both a C89 example and a C++17 example in the directory `examples/`.
You can compile them with:
```
gcc -std=c89 diagctx.c examples/main.c -o diagctx-example
g++ -std=c++17 diagctx.c examples/main.cpp -o diagctx-example
```
The examples will examine an ASCII string line-by-line, and count the number of uppercase letters in each line.
If a line contains a non-ASCII character, an error is raised.

**Input:**
```
Hello World!
ABC def GHI jlk
Hello \x86 World!
\x97 test
\x80\x81\x82
THE END!
```
**Output:**
```
Line 1: 2 upper characters
Line 2: 6 upper characters
ERROR!
  main()
    for_each_line()
      line 3
        count_uppercase_ascii("Hello � World!", 14)
          error: found '\x86' at position 6
ERROR!
  main()
    for_each_line()
      line 4
        count_uppercase_ascii("� test", 6)
          error: found '\x97' at position 0
ERROR!
  main()
    for_each_line()
      line 5
        count_uppercase_ascii("���", 3)
          error: found '\x80' at position 0
Line 6: 6 upper characters
```

## Comparison with catch-and-rethrow idiom

Here are two C++ examples which demonstrate the differences:

```cpp
//////// catch-and-rethrow ////////

int div(int a, int b) {
    if (b == 0)
        throw invalid_argument("div by zero");
    return a / b;
}

int eval(string const& expr) {
    istringstream in{expr};
    int a; char op; int b;
    in >> a >> op >> b;
    try {
        switch (op) {
        ...
        case '/': return div(a, b);
        default: throw invalid_argument("unknown operation");
        }
    } catch (...) {
        throw_with_nested(invalid_argument("while evaluating '" + expr + "'"));
    }
}

//////// diagctx ////////

// using 'debug_ctx' from <examples/main.cpp>

int div(int a, int b) {
    if (b == 0)
        throw invalid_argument("div by zero");
    return a / b;
}

int eval(string const& expr) {
    unsigned msg_id = debug_ctx("while evaluating ', expr, "'");
    istringstream in{expr};
    int a; char op; int b;
    in >> a >> op >> b;
    switch (op) {
    ...
    case '/': return div(a, b);
    default: throw invalid_argument("unknown operation");
    }
    diagctx_pop(msg_id);
}
```

The most important difference is that with catch-and-rethrow, the context is created
only on errors, so it may be quicker when no exceptions occur.

However, diagctx provides two things which catch-and-rethrow cannot provide.

First, you can access the context even when no error occured. For instance,
you may want to emit a warning, which does not stop the program flow. With diagctx, this is easy:
```cpp
void warn(unsigned last_msg_id, string_view msg) {
    cerr << "Warning! " << msg << "\n";
    cerr << "Context:\n";
    int nesting_lvl = 1;
    diagctx_get(-1, debug_handler, &indent_level); // same 'debug_handler' as <examples/main.cpp>
}
```
With catch-and-rethrow, it cannot be done.

Second, memory usage is controlled. With `diagctx_init()`, you specify a maximum number of stored messages.
If this number is reached, then `diagctx_push()` will return `NULL`.
However, the rest of the library remains the same. `diagctx_pop()` will still be correct.
`diagctx_get` will call the handler with `NULL` to represent non-stored messages.

To see this, you can edit the main function of either `examples/main.c` or `examples/main.cpp`,
and specify a smaller capacity. For example, if we use `capacity == 3`, we have the following output:
```
Line 1: 2 upper characters
Line 2: 6 upper characters
ERROR!
  main()
    for_each_line()
      line 3
        ??? (no memory available)
          ??? (no memory available)
ERROR!
  main()
    for_each_line()
      line 4
        ??? (no memory available)
          ??? (no memory available)
ERROR!
  main()
    for_each_line()
      line 5
        ??? (no memory available)
          ??? (no memory available)
Line 6: 6 upper characters
```

Memory usage of nested exceptions is uncontrollable, and so it may be not acceptable for low-memory systems.



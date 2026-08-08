# Banglish

> A small, dynamically typed scripting language with Bangla-inspired keywords written in Latin script.

Banglish is a lightweight scripting language implemented in **C99** with a **tree-walk interpreter** and **zero external runtime dependencies**. It is designed as a compiler/interpreter-design project while still providing a practical scripting experience with variables, functions, closures, arrays, maps, control flow, a standard library, and developer diagnostics.

---

## Highlights

-  Tree-walk interpreter written in C99
- 🇧🇩 Bangla-inspired keywords using Latin characters
-  Dynamically typed values
-  Numbers, strings, booleans, null, arrays, maps, and functions
-  `jodi`, `nahole`, `jotokhon`, and `ghuro` control flow
-  First-class functions and closures
-  Recursion
-  Dynamic heterogeneous arrays
-  String-keyed maps/dictionaries
-  STL-inspired built-in functions
-  Interactive input/output
-  Lexer, parser, AST, semantic analysis, and runtime diagnostics
-  Colored diagnostic/tool output
-  AST, token, semantic, and diagnosis inspection modes
-  Simple Makefile-based build system
-  Zero external dependencies

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Project Structure](#project-structure)
3. [Language Overview](#language-overview)
4. [Comments](#comments)
5. [Variables and Constants](#variables-and-constants)
6. [Data Types](#data-types)
7. [Operators](#operators)
8. [Control Flow](#control-flow)
9. [Functions](#functions)
10. [Arrays](#arrays)
11. [Maps](#maps)
12. [Input and Output](#input-and-output)
13. [Standard Library](#standard-library)
14. [Type Conversion](#type-conversion)
15. [Error Handling](#error-handling)
16. [Interpreter and Diagnostic Modes](#interpreter-and-diagnostic-modes)
17. [Example Program](#example-program)
18. [Keyword Reference](#keyword-reference)
19. [Build Commands](#build-commands)
20. [Architecture](#architecture)
21. [Current Limitations](#current-limitations)

---

# Quick Start

## Requirements

You only need:

- A C99-compatible compiler
- `make`

No third-party libraries are required.

## Build

```bash
make
```

The executable is generated at:

```text
bin/banglish
```

## Run a program

```bash
./bin/banglish examples/source.bs
```

For example:

```js
dekhao("Hello, Dhaka!");
```

Output:

```text
Hello, Dhaka!
```

Running the executable without a source argument launches the built-in demonstration program.

---

#  Project Structure

```text
banglish-diag/
├── Makefile
├── README.md
├── bin/
│   └── banglish
├── build/
│   ├── *.o
│   └── ...
├── include/
│   └── banglish.h
├── src/
│   ├── ast.c
│   ├── builtins.c
│   ├── diagnostics.c
│   ├── env.c
│   ├── interpreter.c
│   ├── lexer.c
│   ├── main.c
│   ├── parser.c
│   └── value.c
└── examples/
    ├── source.bs
    ├── source-comprehensive.bs
    ├── lexical_error.bs
    ├── syntax_error.bs
    └── semantic_error.bs
```

### Core components

| Component | Responsibility |
|---|---|
| `lexer.c` | Converts source code into tokens |
| `parser.c` | Builds the syntax tree |
| `ast.c` | AST representation and utilities |
| `interpreter.c` | Executes the AST |
| `env.c` | Variables, scopes, and environments |
| `value.c` | Runtime value representation |
| `builtins.c` | Standard-library/native functions |
| `diagnostics.c` | Errors and developer diagnostics |
| `main.c` | CLI and execution entry point |

---

#  Language Overview

Banglish uses familiar programming concepts but replaces many common English keywords with Bangla-inspired equivalents.

For example:

```js
dhoro x = 10;

jodi (x > 5) {
    dekhao("x is large");
}
```

Instead of:

```text
let x = 10;

if (x > 5) {
    print("x is large");
}
```

The language is dynamically typed, so variables do not require explicit type declarations.

---

#  Comments

## Single-line comments

```js
// This is a comment

dhoro x = 10; // This is also a comment
```

## Multi-line comments

```js
/*
   This is a
   multi-line comment.
*/
```

---

#  Variables and Constants

## Mutable variables

Use `dhoro` to declare a mutable variable.

```js
dhoro x = 10;

x = x + 5;

dekhao(x);
```

Output:

```text
15
```

## Constants

Use `sthir` for immutable values.

```js
sthir PI = 3.14159;
```

Trying to modify a constant produces a runtime error:

```js
PI = 4;
```

```text
[Runtime Error] Line 2: Cannot assign to constant 'PI'.
```

## Compound assignment

The language supports:

```js
score += 5;
score -= 3;
```

These are equivalent to:

```js
score = score + 5;
score = score - 3;
```

Variables are **block-scoped**. A variable declared inside `{ }` is not visible outside that block.

---

#  Data Types

Banglish currently supports the following runtime types:

| Type | Example | Description |
|---|---|---|
| Number | `10`, `3.14`, `-7` | 64-bit floating-point number |
| String | `"Dhaka"` | Text |
| Boolean | `shotto`, `mitha` | True or false |
| Null | `kichu_na` | Represents no value |
| Array | `[1, 2, 3]` | Dynamic, heterogeneous, 0-indexed collection |
| Map | `{"naam": "Rahim"}` | String-keyed dictionary |
| Function | `kaj(x) { ... }` | First-class callable value |

Example:

```js
dhoro name = "Rahim";
dhoro age = 22;
dhoro student = shotto;
dhoro nothing = kichu_na;

dhoro mixed = [
    1,
    "two",
    3.0,
    shotto,
    kichu_na
];
```

Numbers are internally represented as 64-bit floating-point values.

---

#  Operators

## Arithmetic

```text
+   -   *   /   %
```

Example:

```js
dekhao(10 + 5);
dekhao(10 - 5);
dekhao(10 * 5);
dekhao(10 / 5);
dekhao(10 % 3);
```

The `+` operator also supports string concatenation:

```js
dekhao("Age: " + 22);
```

## Comparison

```text
==   !=   <   <=   >   >=
```

Numbers are compared numerically and strings lexicographically.

```js
dekhao(10 > 5);
dekhao("abc" == "abc");
```

## Logical operators

| Operator | Meaning |
|---|---|
| `ebong` | AND |
| `othoba` | OR |
| `noi` | NOT |

Example:

```js
dhoro a = shotto;
dhoro b = mitha;

dekhao(a ebong b);   // mitha
dekhao(a othoba b);  // shotto
dekhao(noi a);       // mitha
```

## Operator precedence

From low to high:

```text
assignment
othoba
ebong
== !=
< <= > >=
+ -
* / %
unary - / noi
call / index / member access
```

---

#  Control Flow

## If / Else If / Else

```js
dhoro age = 20;

jodi (age < 13) {
    dekhao("Shishu");
} nahole_jodi (age < 20) {
    dekhao("Kishor");
} nahole {
    dekhao("Prapto boyoshko");
}
```

| Banglish | Equivalent |
|---|---|
| `jodi` | `if` |
| `nahole_jodi` | `else if` |
| `nahole` | `else` |

---

## While loop

```js
dhoro i = 0;

jotokhon (i < 5) {
    dekhao(i);
    i = i + 1;
}
```

`jotokhon` means `while`.

---

## For loop

```js
ghuro dhoro i = 0; i < 5; i = i + 1 {
    dekhao(i);
}
```

Syntax:

```text
ghuro <initialization>; <condition>; <increment> { <body> }
```

The increment is a bare expression and there is no semicolon after it.

---

## Break

Use `thamok` to exit a loop.

```js
ghuro dhoro i = 0; i < 10; i = i + 1 {
    jodi (i == 5) {
        thamok;
    }

    dekhao(i);
}
```

## Continue

Use `chaliye_jao` to skip to the next iteration.

```js
ghuro dhoro i = 0; i < 10; i = i + 1 {
    jodi (noi (i % 2 == 0)) {
        chaliye_jao;
    }

    dekhao(i);
}
```

---

#  Functions

## Function declaration

Use `kaj` to define a function.

```js
kaj jog(a, b) {
    ferot a + b;
}

dekhao(jog(3, 4));
```

Output:

```text
7
```

`ferot` returns a value from a function.

If a function does not explicitly return a value, it implicitly returns `kichu_na`.

---

## Recursion

Functions can call themselves.

```js
kaj factorial(n) {
    jodi (n <= 1) {
        ferot 1;
    } nahole {
        ferot n * factorial(n - 1);
    }
}

dekhao(factorial(5));
```

Output:

```text
120
```

---

## First-class functions

Functions are values and can be:

- Assigned to variables
- Passed as arguments
- Returned from functions
- Called later

Anonymous functions are created with `kaj(...)`.

```js
dhoro square = kaj(x) {
    ferot x * x;
};

dekhao(square(9));
```

Output:

```text
81
```

---

## Closures

Functions can capture variables from their surrounding scope.

```js
kaj make_counter() {
    dhoro count = 0;

    ferot kaj() {
        count = count + 1;
        ferot count;
    };
}

dhoro counter = make_counter();

dekhao(counter()); // 1
dekhao(counter()); // 2
dekhao(counter()); // 3
```

---

# Arrays

Arrays are dynamic, heterogeneous, and zero-indexed.

```js
dhoro nums = [42, 7, 19, 3];

dekhao(nums[0]);
```

Output:

```text
42
```

## Array assignment

```js
nums[0] = 100;

dekhao(nums);
```

## Length

```js
dekhao(doirgho(nums));
```

## Negative indexing

Negative indices count backward from the end.

```js
dhoro arr = [1, 2, 3];

dekhao(arr[-1]);
```

Output:

```text
3
```

## Nested arrays

```js
dhoro grid = [
    [1, 2],
    [3, 4],
    "Dhaka"
];

dekhao(grid[0][1]);
```

Output:

```text
2
```

---

#  Maps

Maps are string-keyed dictionaries.

```js
dhoro person = {
    "naam": "Rahim",
    "bhoysh": 22
};

dekhao(person["naam"]);
```

Output:

```text
Rahim
```

## Add or update a key

```js
person["city"] = "Dhaka";
```

## Dot notation

String-like keys can also be accessed using dot syntax:

```js
dekhao(person.naam);
```

This is equivalent to:

```js
dekhao(person["naam"]);
```

## Nested maps

Maps can contain any supported value type.

```js
dhoro student = {
    "naam": "Karim",
    "grades": [90, 85, 78],
    "address": {
        "city": "Dhaka",
        "zip": "1207"
    }
};

dekhao(student["address"]["city"]);
```

---

#  Input and Output

## `dekhao(...)`

Prints any number of values separated by spaces and ends with a newline.

```js
dekhao("Hello");
dekhao("Name:", "Rahim");
dekhao(10, 20, 30);
```

## `shono()`

Reads one line of input as a string.

```js
dekhao("What is your name?");

dhoro name = shono();

dekhao("Hello,", name);
```

## `shono_shonkhya()`

Reads one line and parses it as a number.

```js
dekhao("Enter your age:");

dhoro age = shono_shonkhya();

dekhao("Next year:", age + 1);
```

---

# Standard Library

Banglish includes an STL-inspired set of native functions for arrays and strings.

| Function | Similar to | Description |
|---|---|---|
| `doirgho(x)` | `size()` / `len()` | Length of string, array, or map |
| `shesh_jog(arr, val)` | `push_back()` | Appends a value to an array |
| `shesh_bad(arr)` | `pop_back()` | Removes and returns the last array element |
| `sajao(arr)` | `sort()` | Sorts an array in place |
| `ultho(x)` | `reverse()` | Reverses an array or string |
| `khojo(x, target)` | `find()` | Finds a value and returns its index |
| `kato(x, start, end)` | `slice()` | Extracts `[start:end)` |
| `bhoro(arr, val, count)` | `fill_n()` | Appends repeated values |

## Example

```js
dhoro nums = [5, 3, 9, 1];

sajao(nums);
dekhao(nums);
// [1, 3, 5, 9]

ultho(nums);
dekhao(nums);
// [9, 5, 3, 1]

dekhao(khojo(nums, 5));
// 1

shesh_jog(nums, 100);

dekhao(shesh_bad(nums));
// 100

dekhao(kato(nums, 1, 3));
// [5, 3]

dhoro filled = [];

bhoro(filled, 0, 5);

dekhao(filled);
// [0, 0, 0, 0, 0]
```

---

# String Operations

The same collection helpers can operate on strings where supported.

```js
dhoro s = "Banglish";

dekhao(doirgho(s));
dekhao(ultho(s));
dekhao(kato(s, 0, 8));
dekhao(khojo(s, "Script"));
```

Example output:

```text
14
tpircShsilgnaB
Banglish
8
```

---

#  Type Conversion

## `shonkhya(x)`

Converts supported values such as strings or booleans to a number.

```js
dekhao(shonkhya("42") + 8);
```

Output:

```text
50
```

## `shobdo(x)`

Converts a value to its string representation.

```js
dekhao(shobdo(42) + "!");
```

Output:

```text
42!
```

---

# Error Handling

Banglish currently does not provide `try/catch`.

When an error occurs:

1. Execution stops.
2. A diagnostic is printed to `stderr`.
3. The interpreter exits with an appropriate exit code.

## Exit codes

| Code | Meaning |
|---:|---|
| `0` | Successful execution |
| `64` | Invalid CLI usage |
| `65` | Parse/syntax error |
| `70` | Runtime error |
| `74` | Source file could not be opened |

Example:

```js
sthir PI = 3.14;

PI = 4;
```

Produces:

```text
[Runtime Error] Line 3: Cannot assign to constant 'PI'.
```

---

#  Interpreter and Diagnostic Modes

The interpreter can expose different stages of the language pipeline.

## Normal execution

```bash
./bin/banglish examples/source.bs
```

## Token inspection

```bash
./bin/banglish --token examples/source.bs
```

Displays the tokens produced by the lexer.

Useful for debugging lexical analysis.

## AST inspection

```bash
./bin/banglish --ast examples/source.bs
```

Displays the Abstract Syntax Tree generated by the parser.

Useful for understanding parsing and syntax structure.

## Semantic analysis

```bash
./bin/banglish --semantic examples/source.bs
```

Runs the semantic-analysis stage and reports semantic problems.

## Full diagnosis

```bash
./bin/banglish --diagnosis examples/source.bs
```

Runs the diagnostic pipeline and presents the available analysis information.

These modes make the project useful not only as a scripting language, but also as a demonstration of a complete language-processing pipeline.

---

#  Example Programs

The repository includes examples for different parts of the language:

```text
examples/
├── source.bs
├── source-comprehensive.bs
├── lexical_error.bs
├── syntax_error.bs
└── semantic_error.bs
```

The error examples are useful for testing the diagnostic system.

---

#  Full Example Program

```js
// Recursive Fibonacci + array + map demo

kaj fibonacci(n) {
    jodi (n < 2) {
        ferot n;
    }

    ferot fibonacci(n - 1) + fibonacci(n - 2);
}

dhoro fib_results = [];

ghuro dhoro i = 0; i < 10; i = i + 1 {
    shesh_jog(fib_results, fibonacci(i));
}

dekhao("Fibonacci 0-9:", fib_results);

dhoro students = [
    {"naam": "Rahim", "score": 78},
    {"naam": "Karim", "score": 92},
    {"naam": "Salma", "score": 85}
];

ghuro dhoro i = 0; i < doirgho(students); i = i + 1 {
    dhoro s = students[i];

    jodi (s["score"] >= 90) {
        dekhao(s["naam"], "-> Excellent");
    } nahole_jodi (s["score"] >= 80) {
        dekhao(s["naam"], "-> Good");
    } nahole {
        dekhao(s["naam"], "-> Needs Improvement");
    }
}
```

Output:

```text
Fibonacci 0-9: [0, 1, 1, 2, 3, 5, 8, 13, 21, 34]
Rahim -> Needs Improvement
Karim -> Excellent
Salma -> Good
```

---

#  Keyword Reference

| Banglish | English equivalent |
|---|---|
| `dhoro` | `let` / `var` |
| `sthir` | `const` |
| `kaj` | `function` |
| `ferot` | `return` |
| `jodi` | `if` |
| `nahole_jodi` | `else if` |
| `nahole` | `else` |
| `jotokhon` | `while` |
| `ghuro` | `for` |
| `thamok` | `break` |
| `chaliye_jao` | `continue` |
| `shotto` | `true` |
| `mitha` | `false` |
| `kichu_na` | `null` |
| `ebong` | `and` |
| `othoba` | `or` |
| `noi` | `not` |

---

#  Build Commands

## Build

```bash
make
```

## Run

```bash
./bin/banglish examples/source.bs
```

## Clean

```bash
make clean
```

This removes generated `build/` and `bin/` contents.

## Rebuild from scratch

```bash
make rebuild
```

---

### Language pipeline

**1. Lexical analysis**

The lexer reads source code and produces tokens.

**2. Parsing**

The parser consumes tokens and builds an Abstract Syntax Tree.

**3. Semantic analysis**

The diagnostic system can inspect the program for semantic problems before execution.

**4. Interpretation**

The tree-walk interpreter evaluates the AST directly.

**5. Runtime**

The runtime manages values, environments, functions, arrays, maps, and built-in functions.

---

#  Current Limitations

The current language intentionally remains small.

- No `try/catch` exception system
- No user-defined classes or objects
- No module/import system
- No static type system
- No package manager
- No garbage-collector-based runtime described as part of the current language specification
- The standard library is intentionally small
- The interpreter executes the AST directly rather than generating native machine code

These limitations keep the implementation compact and make the project suitable for studying interpreter and compiler-design concepts.

---

#  Project Goals

Banglish is intended to demonstrate the major stages involved in building a programming language:

```text
Source Code -> Lexical Analysis -> Parsing -> AST Construction -> Semantic Diagnostics -> Tree-Walk Interpretation -> Runtime Execution
```

The project combines these language-design concepts with a Bangla-inspired syntax to create a language that is both educational and practical to experiment with.

---

## Banglish

**Bangla-inspired syntax.  
C99 implementation.  
Zero external dependencies.  
A complete little language.**

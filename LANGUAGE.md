# BanglishScript

BanglishScript is a small, dynamically-typed scripting language with Bangla-inspired keywords written in Latin script. It runs on a tree-walk interpreter built in C99 with zero external dependencies.

```
./banglish source.bs
```

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Comments](#comments)
3. [Variables & Constants](#variables--constants)
4. [Data Types](#data-types)
5. [Operators](#operators)
6. [Control Flow](#control-flow)
7. [Functions](#functions)
8. [Arrays](#arrays)
9. [Maps (Dictionaries)](#maps-dictionaries)
10. [Input / Output](#input--output)
11. [Built-in Standard Library](#built-in-standard-library)
12. [Error Handling & Exit Codes](#error-handling--exit-codes)
13. [Full Example Program](#full-example-program)
14. [Keyword Reference](#keyword-reference)

---

## Quick Start

Save a file as `hello.bs`:

```js
dekhao("Hello, Dhaka!");
```

Run it:

```bash
./banglish hello.bs
```

Output:

```
Hello, Dhaka!
```

Running `./banglish` with no arguments launches a built-in demo program showing off every language feature.

---

## Comments

```js
// single-line comment

/*
  multi-line
  comment
*/
```

---

## Variables & Constants

| Keyword | Meaning                          |
|---------|-----------------------------------|
| `dhoro`  | Declares a mutable variable       |
| `sthir`  | Declares a constant (immutable)   |

```js
dhoro x = 10;
x = x + 5;          // ok, dhoro is mutable
dekhao(x);           // 15

sthir PI = 3.14159;
PI = 4;               // Runtime Error: Cannot assign to constant 'PI'.
```

Variables are block-scoped — a variable declared inside `{ }` is not visible outside it.

Shorthand compound assignment is supported:

```js
dhoro score = 10;
score += 5;   // score = score + 5  -> 15
score -= 3;   // score = score - 3  -> 12
```

---

## Data Types

| Type        | Example                          | Notes                                   |
|-------------|-----------------------------------|-------------------------------------------|
| Number       | `10`, `3.14`, `-7`               | All numbers are 64-bit floats internally  |
| String        | `"Dhaka"`                          | Supports escapes: `\n`, `\t`, `\"`, `\\`  |
| Boolean       | `shotto` (true), `mitha` (false)  |                                            |
| Null           | `kichu_na`                         | Represents "nothing"                      |
| Array          | `[1, "Dhaka", shotto, [1,2]]`    | Dynamic, heterogeneous, 0-indexed         |
| Map              | `{"naam": "Rahim", "bhoysh": 22}` | String-keyed dictionary                   |
| Function     | `kaj(x) { ferot x * 2; }`         | First-class, can be stored/passed         |

```js
dhoro name = "Rahim";
dhoro age = 22;
dhoro is_student = shotto;
dhoro nothing = kichu_na;
dhoro mixed = [1, "two", 3.0, shotto, kichu_na];
```

---

## Operators

### Arithmetic
```
+   -   *   /   %
```
`+` also concatenates when either side is a string:
```js
dekhao("Age: " + 22);        // "Age: 22"
```

### Comparison
```
==  !=  <  <=  >  >=
```
Numbers compare numerically; strings compare lexicographically.

### Logical
| Keyword | Meaning |
|---------|---------|
| `ebong`  | logical AND |
| `othoba` | logical OR  |
| `noi`     | logical NOT (unary) |

```js
dhoro a = shotto;
dhoro b = mitha;

dekhao(a ebong b);   // mitha
dekhao(a othoba b);  // shotto
dekhao(noi a);         // mitha
```

### Operator Precedence (low → high)
```
assignment
othoba (or)
ebong (and)
==  !=
<  <=  >  >=
+  -
*  /  %
unary - / noi
call / index  ( ) [ ] .
```

---

## Control Flow

### If / Else If / Else

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

- `jodi` = `if`
- `nahole_jodi` = `else if`
- `nahole` = `else`

### While Loop

```js
dhoro i = 0;
jotokhon (i < 5) {
    dekhao(i);
    i = i + 1;
}
```

### For Loop

```js
ghuro dhoro i = 0; i < 5; i = i + 1 {
    dekhao(i);
}
```

Syntax: `ghuro <init>; <condition>; <increment> { <body> }`
Note there is **no** semicolon after the increment clause, and the increment is a bare expression (not a statement).

### Break & Continue

```js
ghuro dhoro i = 0; i < 10; i = i + 1 {
    jodi (i == 5) {
        thamok;          // break
    }
    jodi (noi (i % 2 == 0)) {
        chaliye_jao;    // continue
    }
    dekhao(i);
}
// prints: 0 2 4
```

- `thamok` = `break`
- `chaliye_jao` = `continue`

---

## Functions

### Declared Functions

```js
kaj jog(a, b) {
    ferot a + b;
}

dekhao(jog(3, 4));   // 7
```

`ferot` returns a value from a function (like `return`). A function with no `ferot` implicitly returns `kichu_na`.

### Recursion

```js
kaj factorial(n) {
    jodi (n <= 1) {
        ferot 1;
    } nahole {
        ferot n * factorial(n - 1);
    }
}

dekhao(factorial(5));   // 120
```

### First-Class Functions & Closures

Functions are values — they can be assigned to variables, passed as arguments, and returned from other functions. Anonymous functions use `kaj(...)` without a name:

```js
dhoro square = kaj(x) { ferot x * x; };
dekhao(square(9));   // 81
```

Closures capture their surrounding scope:

```js
kaj make_counter() {
    dhoro count = 0;
    ferot kaj() {
        count = count + 1;
        ferot count;
    };
}

dhoro counter = make_counter();
dekhao(counter());   // 1
dekhao(counter());   // 2
dekhao(counter());   // 3
```

---

## Arrays

```js
dhoro nums = [42, 7, 19, 3];

dekhao(nums[0]);        // 42
nums[0] = 100;             // index assignment
dekhao(nums);              // [100, 7, 19, 3]

dekhao(doirgho(nums));   // 4 (length)
```

Negative indices count from the end:

```js
dhoro arr = [1, 2, 3];
dekhao(arr[-1]);   // 3
```

Arrays can be nested and heterogeneous:

```js
dhoro grid = [[1, 2], [3, 4], "Dhaka"];
dekhao(grid[0][1]);   // 2
```

---

## Maps (Dictionaries)

```js
dhoro person = {"naam": "Rahim", "bhoysh": 22};

dekhao(person["naam"]);   // Rahim

person["city"] = "Dhaka";   // add / update a key
dekhao(person);                 // {"naam": "Rahim", "bhoysh": 22, "city": "Dhaka"}
```

Dot syntax is also supported for reading string-literal-like keys:

```js
dekhao(person.naam);   // Rahim (sugar for person["naam"])
```

Maps can hold arrays, nested maps, or any value type:

```js
dhoro student = {
    "naam": "Karim",
    "grades": [90, 85, 78],
    "address": {"city": "Dhaka", "zip": "1207"}
};
dekhao(student["address"]["city"]);   // Dhaka
```

---

## Input / Output

| Function              | Purpose                                |
|-----------------------|-------------------------------------------|
| `dekhao(...)`          | Print any number of values, space-separated, newline at the end |
| `shono()`               | Read a line of input as a string       |
| `shono_shonkhya()` | Read a line of input, parsed as a number |

```js
dekhao("What is your name?");
dhoro name = shono();
dekhao("Hello,", name);

dekhao("Enter your age:");
dhoro age = shono_shonkhya();
dekhao("Next year you'll be", age + 1);
```

---

## Built-in Standard Library

BanglishScript ships with an STL-inspired set of native functions for arrays and strings.

| Function | Equivalent | Description |
|----------|------------|--------------|
| `doirgho(x)` | `size()` / `len()` | Length of a string, array, or map |
| `shesh_jog(arr, val)` | `push_back()` | Appends `val` to the end of `arr` (mutates in place) |
| `shesh_bad(arr)` | `pop_back()` | Removes and returns the last element |
| `sajao(arr)` | `sort()` | Sorts `arr` in place (ascending, numeric or lexicographic) |
| `ultho(x)` | `reverse()` | Reverses an array or string in place / returns reversed string |
| `khojo(x, target)` | `find()` | Returns 0-based index of `target`, or `-1` if not found |
| `kato(x, start, end)` | slice / substring | Extracts `x[start:end)` from an array or string |
| `bhoro(arr, val, count)` | `fill_n()` | Appends `count` copies of `val` to `arr` |

### Examples

```js
dhoro nums = [5, 3, 9, 1];

sajao(nums);
dekhao(nums);              // [1, 3, 5, 9]

ultho(nums);
dekhao(nums);              // [9, 5, 3, 1]

dekhao(khojo(nums, 5));   // 1

shesh_jog(nums, 100);
dekhao(nums);              // [9, 5, 3, 1, 100]

dekhao(shesh_bad(nums));  // 100
dekhao(nums);              // [9, 5, 3, 1]

dekhao(kato(nums, 1, 3)); // [5, 3]

dhoro filled = [];
bhoro(filled, 0, 5);
dekhao(filled);            // [0, 0, 0, 0, 0]
```

### Strings work with the same functions

```js
dhoro s = "BanglishScript";

dekhao(doirgho(s));         // 14
dekhao(ultho(s));            // tpircShsilgnaB
dekhao(kato(s, 0, 8));     // Banglish
dekhao(khojo(s, "Script")); // 8
```

### Type Conversion Helpers

| Function | Description |
|----------|--------------|
| `shonkhya(x)` | Converts a string or boolean to a number |
| `shobdo(x)`  | Converts any value to its string representation |

```js
dekhao(shonkhya("42") + 8);   // 50
dekhao(shobdo(42) + "!");        // "42!"
```

---

## Error Handling & Exit Codes

BanglishScript does not currently have try/catch — errors halt execution and print a diagnostic to `stderr`.

| Exit Code | Meaning |
|-----------|---------|
| `0`  | Successful run |
| `64` | Bad CLI usage (wrong number of arguments) |
| `65` | Parse error (invalid syntax) |
| `70` | Runtime error (undefined variable, type mismatch, division by zero, etc.) |
| `74` | Could not open the source file |

```js
sthir PI = 3.14;
PI = 4;
```
```
[Runtime Error] Line 2: Cannot assign to constant 'PI'.
```

---

## Full Example Program

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
```
Fibonacci 0-9: [0, 1, 1, 2, 3, 5, 8, 13, 21, 34]
Rahim -> Needs Improvement
Karim -> Excellent
Salma -> Good
```

---

## Keyword Reference

| Keyword | English Equivalent |
|---------|----------------------|
| `dhoro` | `let` / `var` |
| `sthir` | `const` |
| `kaj`     | `function` |
| `ferot`  | `return` |
| `jodi`    | `if` |
| `nahole_jodi` | `else if` |
| `nahole` | `else` |
| `jotokhon` | `while` |
| `ghuro`   | `for` |
| `thamok` | `break` |
| `chaliye_jao` | `continue` |
| `shotto`  | `true` |
| `mitha`   | `false` |
| `kichu_na` | `null` |
| `ebong`   | `and` |
| `othoba` | `or` |
| `noi`       | `not` |

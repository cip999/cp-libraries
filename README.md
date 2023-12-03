# cp-libraries | C++ libraries for competitive programming

A collection of utilities to streamline the preparation of competitive programming tasks!

This project takes inspiration from Codeforces' [testlib](https://codeforces.com/testlib),
but is meant to be optimized for use in the Italian Olympiad in Informatics.

## Recommended usage

All the classes and functions live in the custom C++ namespace `cplib`,
with each library defining its own sub-namespace — `cplib::io`, `cplib::val`, etc.

To avoid incurring in ambiguous references, I suggest a setup such as

```cpp
// Standard library includes

#include "validation.hpp" // Or some other header

using namespace cplib;

// Further in the code
ASSERT(val::lte(N, 100));
```

(Notice the absence of `using namespace std;`)

## Feature summary

cp-libraries offers:

- Generic support for input/output — for validators, generators, checkers, interactors.
- An exception-based validation framework.
- TODO: Generator helpers.
- TODO: A checker framework.
- TODO: A graph library.
- TODO: A computational geometry library.

### Input/Output

The `cplib::io::Reader` class:

- Can toggle strict and non-strict read — the former for input validation,
  the latter e.g. for reading the contestant output in the checker.
- Throws scoped exceptions when something goes wrong.
- Provides template methods to read scalars, strings, arrays and matrices
  with just a few characters of code.
- Can read floating point numbers with or without a fixed number of decimals.
- Integrates some validation checks.
- Automatically detects integer overflows (it will fail to read
  $3\,000\,000\,000$ as an `int`).
- Implements the input stream operator as an alias for common methods.

The `cplib::io::Writer` class:

- Provides template methods to write scalars, strings, arrays and matrices
  with just a few characters of code.
- Implements the output stream operator as an alias for common methods.

Read the full documentation [here](#iohpp).

### Validation

The validation library relies on exceptions to signal failures.
A brief list of salient features:

- Combine validation checks with the `ASSERT` macro to get comprehensive error messages.
- Built-in support for enforcing element-wise predicates over iterables.
- Shortcuts for common checks such as "is this array sorted?".
- Validation results can be combined through logical operators and evaluated as booleans.

Read the full documentation [here](#validationhpp).

## Documentation

### `io.hpp`

TODO

### `validation.hpp`

TODO

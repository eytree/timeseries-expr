# timeseries-expr

A small C++17 expression parser/evaluator for arithmetic over `TimeSeries` values stored in an environment map.

- Parses assignments like: `z = a + b - c / 2`
- Supports: identifiers, numbers, `+ - * /`, parentheses, unary minus.
- Compiles to Reverse Polish Notation (RPN) via shunting-yard, then evaluates with a stack machine.

This repo ships with a **stub `TimeSeries`** implementation (vector of doubles) so the parser is testable.
Replace it with your real time series type (and alignment semantics) by implementing the required operators.

## Build & test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Quick usage

```cpp
#include <tsexpr/expr.hpp>

ts::expr::Env env;
env["a"] = TimeSeries{ {1,2,3} };
env["b"] = TimeSeries{ {10,20,30} };
env["c"] = TimeSeries{ {2,4,6} };

ts::expr::execute_assignment("z = a + b - c / 2", env);
```

## Design notes

- Tokenize → Shunting-yard (RPN) → Evaluate.
- Unary minus is represented as an internal `NEG` operator.
- Evaluation uses `std::variant<TimeSeries, double>` to allow scalar literals.
- Assignment currently requires the expression to evaluate to `TimeSeries`.

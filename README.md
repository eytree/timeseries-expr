# timeseries-expr

A small C++17 expression compiler for arithmetic expressions over **variables** and **scalars**:

```text
z = a + b - c / 2
z = `total return` + carry / 2
s = sumproduct(a, b)
```

It compiles expressions into a small **bytecode program** (stack machine). Evaluation is **backend-driven**:
the parser/compiler knows nothing about your `TimeSeries` type.

## Build & test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Using it

See [`examples/toy_backend.cpp`](examples/toy_backend.cpp) for a minimal backend that supports:
- a toy "TimeSeries" as `std::vector<double>`
- scalars (`double`)
- elementwise arithmetic and `sumproduct`

Core API:

```cpp
#include <tsexpr/parser.hpp>

tsexpr::Program p = tsexpr::compile("z = `total return` + carry / 2");
p.execute(backend);
```

Where `backend` provides a small set of operations (load/store, arithmetic, function call dispatch).

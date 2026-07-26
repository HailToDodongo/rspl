# RSPL C++ Backend

Native C++ port of the RSPL transpiler. Parsing is delegated to the existing
JS parser (`scripts/parse.js`); everything downstream runs in C++.

## Build

Requirements: **CMake 3.20+**, **g++ 13+** (or clang++ with C++20), **Node.js 20+**.

```sh
cd cpp

# Debug build (no optimizations, asserts enabled, debug symbols)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Production build (-O3, no asserts, stripped)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Dependencies (nlohmann/json, Catch2) are fetched automatically by CMake.

## Run

From the repo root:

```sh
cpp/build/rspl input.rspl                    # full pipeline → stdout
cpp/build/rspl input.rspl -o output.S        # write to file
cpp/build/rspl input.rspl --no-optimize      # skip optimizer
cpp/build/rspl input.rspl --no-rspq          # raw asm, no RSPQ wrapper
cpp/build/rspl input.rspl --magma            # compile as a magma shader
cpp/build/rspl input.rspl --patch fnA,fnB     # only re-optimize these, patch into existing .S
cpp/build/rspl input.rspl --ast-dump         # dump parsed AST (debug)
cpp/build/rspl input.rspl -D FOO=42          # preprocessor define
cpp/build/rspl input.rspl --reorder          # enable instruction reorder annealing
cpp/build/rspl input.rspl --opt-time=60      # optimizer time budget in seconds
```

The binary invokes `node scripts/parse.js` internally. Set `RSPL_PARSE_JS` env var
to override the script path:

```sh
RSPL_PARSE_JS=/path/to/custom/parse.js cpp/build/rspl input.rspl
```

## Tests

```sh
cpp/build/rspl_tests
```

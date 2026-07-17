# Building from Source

## Requirements

- CMake ≥ 3.20
- A C++20 compiler: GCC ≥ 12, Clang ≥ 15, or MSVC (Visual Studio 2022)
- On Linux: `pthread` (usually present by default)
- On Windows: `psapi.lib` (part of the Windows SDK, linked automatically)

## Basic Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Using CMake Presets

```bash
cmake --preset linux-debug      # Linux, Debug, sanitizers on, warnings-as-errors
cmake --build --preset linux-debug
ctest --preset linux-debug

cmake --preset linux-release    # Linux, Release, benchmarks enabled
cmake --preset windows-msvc-release
```

## CMake Options

| Option | Default | Description |
|--------|---------|--------------|
| `RMG_BUILD_SHARED` | `OFF` | Build as a shared library instead of static |
| `RMG_BUILD_TESTS` | `ON` | Build the GoogleTest-based test suite |
| `RMG_BUILD_EXAMPLES` | `ON` | Build the example applications |
| `RMG_BUILD_BENCHMARKS` | `OFF` | Build the Google Benchmark suite |
| `RMG_BUILD_TOOLS` | `ON` | Build `rmg-cli` and `rmg-baseline-generator` |
| `RMG_ENABLE_SANITIZERS` | `OFF` | Enable ASan + UBSan (GCC/Clang only) |
| `RMG_WARNINGS_AS_ERRORS` | `OFF` | Treat compiler warnings as errors |
| `RMG_ENABLE_DOXYGEN` | `OFF` | Enable the Doxygen documentation target |

## Running Tests

```bash
ctest --test-dir build --output-on-failure
```

## Running Benchmarks

```bash
cmake -S . -B build -DRMG_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/benchmarks/bench_memory_scanner
./build/benchmarks/bench_hash_provider
./build/benchmarks/bench_hook_detector
```

## Installing

```bash
cmake --install build --prefix /usr/local
```

This installs headers to `<prefix>/include/rmg/`, the library to `<prefix>/lib/`, and CMake package config files enabling downstream `find_package(RuntimeMemoryGuardian)`.
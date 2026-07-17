# Contributing to Runtime Memory Guardian

Thank you for considering contributing! This document describes how to propose changes, the coding standards we follow, and how to extend the library to new platforms.

## Before You Start

- Please read the [Code of Conduct](CODE_OF_CONDUCT.md).
- For anything beyond a small fix, please open an issue first to discuss the approach — this avoids wasted effort on both sides.
- Security vulnerabilities must **not** be reported via public issues; see [SECURITY.md](SECURITY.md).

## Development Setup

```bash
git clone https://github.com/Gabryx412-coder/RuntimeMemoryGuardian.git
cd RuntimeMemoryGuardian
cmake --preset linux-debug   # or windows-msvc-release on Windows
cmake --build --preset linux-debug
ctest --preset linux-debug
```

See [docs/guides/BUILDING.md](docs/guides/BUILDING.md) for the full list of CMake options and presets.

## Coding Standards

- **C++20**, no compiler-specific extensions (`CMAKE_CXX_EXTENSIONS OFF`).
- Format code with `clang-format` (config in `.clang-format`) before submitting: `clang-format -i <files>`.
- Code must pass `clang-tidy` with the project's `.clang-tidy` configuration.
- Every public class/function must have a Doxygen comment explaining **why it exists**, not just what it does.
- Prefer `rmg::core::Result<T>` over exceptions for fallible operations, consistent with the rest of the codebase.
- New platform-facing code must go through the `rmg::platform::IPlatformTraits` interface — no OS headers outside `include/rmg/platform/` or `src/platform/`.
- Every new non-trivial component should have corresponding unit tests under `tests/unit/`.

## Adding Support for a New Platform

1. Create `src/platform/<platform>/` mirroring the structure of `src/platform/windows/` or `src/platform/linux/`.
2. Implement a concrete `IPlatformTraits` subclass covering `enumerateRegions`, `enumerateModules`, `readMemory`, `queryProtection`, and `pageSize`.
3. Implement `ProcessHandle::open`/`openSelf` for the new platform.
4. Add a factory function (e.g. `create<Platform>PlatformTraits()`) and wire it into `src/platform/platform_factory.cpp` behind a new `RMG_PLATFORM_<PLATFORM>` define (add the detection to `cmake/RMGPlatform.cmake`).
5. Add the new platform to the CI matrix (see `.github/workflows/`).

## Pull Request Process

1. Fork the repository and create a feature branch.
2. Ensure `ctest` passes locally, and that `clang-format`/`clang-tidy` report no issues.
3. Update `CHANGELOG.md` under `[Unreleased]`.
4. Open a pull request with a clear description of the change and its motivation.
5. A maintainer will review; please be responsive to review feedback.

## Reporting Bugs

Please include: RMG version, OS/compiler/version, a minimal reproduction, and the actual vs. expected behavior.
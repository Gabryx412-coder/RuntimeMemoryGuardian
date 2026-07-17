# Runtime Memory Guardian

[![CI (Linux)](https://github.com/Gabryx412-coder/RuntimeMemoryGuardian/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/Gabryx412-coder/RuntimeMemoryGuardian/actions/workflows/ci-linux.yml)
[![CI (Windows)](https://github.com/Gabryx412-coder/RuntimeMemoryGuardian/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Gabryx412-coder/RuntimeMemoryGuardian/actions/workflows/ci-windows.yml)
[![Code Quality](https://github.com/Gabryx412-coder/RuntimeMemoryGuardian/actions/workflows/code-quality.yml/badge.svg)](https://github.com/Gabryx412-coder/RuntimeMemoryGuardian/actions/workflows/code-quality.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

**Runtime Memory Guardian (RMG)** is a modern C++20 library for **defensive, read-only runtime memory monitoring** on Windows and Linux. It helps applications detect signs of runtime tampering — modified code sections, suspicious inline/IAT/EAT hooks, and unexpected module loads — using documented, non-invasive techniques.

RMG is built for security engineers, anti-cheat/anti-tamper developers, and anyone who wants their application to notice when its own memory has been altered at runtime.

## Design Principles

- **Purely defensive.** RMG only reads memory and reports observations. It never patches, injects, or modifies anything in the target process.
- **Cross-platform by design.** A single `IPlatformTraits` abstraction backs both the Windows and Linux implementations; the domain logic (integrity checking, hook detection, module monitoring) is written once.
- **No hidden dependencies.** The core library has zero third-party dependencies — only the C++20 standard library. Tests and benchmarks fetch GoogleTest/Google Benchmark solely for development purposes.
- **Fails loudly, never silently.** Every fallible operation returns `rmg::core::Result<T>` (`std::expected`-based); there are no hidden exceptions to catch.

## Features

- 🔒 **Integrity baselines** — hash code sections (SHA-256 by default) and detect any deviation later.
- 🔍 **Memory scanning** — enumerate and read memory regions with flexible filters (executable-only, W^X detection, per-module).
- 🪝 **Hook detection** — recognize documented inline-hook opcode patterns (`JMP`, `JMP [rip+disp32]`, `PUSH;RET` trampolines); IAT/EAT validation logic is implemented and ready for the planned image-format parser (see [docs/architecture/HOOK_DETECTOR.md](docs/architecture/HOOK_DETECTOR.md)).
- 📦 **Module monitoring** — get notified when modules are loaded or unloaded unexpectedly.
- ⚙️ **Continuous or on-demand monitoring** — run a background polling loop via `ProcessMonitor`, or drive checks manually.
- 🖥️ **CLI tooling** — `rmg-cli` for ad-hoc checks, `rmg-baseline-generator` for producing baselines to ship with your application.

## Quick Start

```cpp
#include <rmg/rmg.hpp>
#include <cstdio>

int main() {
    auto guardian = rmg::api::RuntimeMemoryGuardian::createForSelf();
    if (!guardian) {
        std::fprintf(stderr, "%s\n", guardian.error().toDiagnosticString().c_str());
        return 1;
    }

    if (auto result = guardian->establishBaseline(); !result) {
        std::fprintf(stderr, "%s\n", result.error().toDiagnosticString().c_str());
        return 1;
    }

    auto report = guardian->checkIntegrity();
    if (report && report->isValid) {
        std::printf("No tampering detected.\n");
    }
}
```

See [`examples/`](examples/) for five progressively richer usage scenarios, and [docs/guides/GETTING_STARTED.md](docs/guides/GETTING_STARTED.md) for a full walkthrough.

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

See [docs/guides/BUILDING.md](docs/guides/BUILDING.md) for all CMake options, platform requirements, and CMake presets.

## Documentation

- [Getting Started](docs/guides/GETTING_STARTED.md)
- [Building from Source](docs/guides/BUILDING.md)
- [Integration Guide](docs/guides/INTEGRATION_GUIDE.md)
- [Architecture Overview](docs/architecture/OVERVIEW.md)

## Supported Platforms

| Platform | Status |
|----------|--------|
| Windows 10/11 (x64) | ✅ Supported |
| Linux (x64, glibc) | ✅ Supported |
| macOS | ❌ Not currently supported |

## Contributing

Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md) for coding standards, the PR process, and how to add support for a new platform. Please also review our [Code of Conduct](CODE_OF_CONDUCT.md).

## Security

If you discover a security vulnerability, please follow the responsible disclosure process described in [SECURITY.md](SECURITY.md). Please do not open a public issue for security-sensitive reports.

## License

Runtime Memory Guardian is licensed under the [MIT License](LICENSE).
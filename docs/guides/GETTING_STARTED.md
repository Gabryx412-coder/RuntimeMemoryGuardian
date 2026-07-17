# Getting Started

This guide walks through the most common use case: monitoring an application's own memory for tampering.

## 1. Add RMG to Your Project

Using `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
    RuntimeMemoryGuardian
    GIT_REPOSITORY https://github.com/Gabryx412-coder/RuntimeMemoryGuardian.git
    GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(RuntimeMemoryGuardian)

target_link_libraries(your_app PRIVATE RMG::rmg)
```

Or, if already installed system-wide:

```cmake
find_package(RuntimeMemoryGuardian REQUIRED)
target_link_libraries(your_app PRIVATE RMG::rmg)
```

## 2. Establish a Baseline

```cpp
#include <rmg/rmg.hpp>

auto guardian = rmg::api::RuntimeMemoryGuardian::createForSelf();
if (!guardian) { /* handle guardian.error() */ }

if (auto result = guardian->establishBaseline(); !result) {
    /* handle result.error() */
}
```

Call this once, early in your application's lifecycle — ideally as soon as possible after startup, before any attacker has a realistic window to tamper with your code.

## 3. Check Integrity On-Demand

```cpp
auto report = guardian->checkIntegrity();
if (report && !report->isValid) {
    for (const auto& tampered : report->tamperedSections) {
        // React: log, alert, terminate, etc.
    }
}
```

## 4. Or, Monitor Continuously

```cpp
guardian->monitor().onEvent.connect([](const rmg::process::MonitorEvent& event) {
    // Handle IntegrityViolation / HookDetected / ModuleLoaded / ModuleUnloaded
});
guardian->monitor().start();

// ... later, at shutdown:
guardian->monitor().stop();
```

## 5. Persist a Baseline Across Restarts

Generating a baseline from a process that has *just* started is more trustworthy than one generated from a process that has been running for a while. Use `rmg-baseline-generator` at build/packaging time against a known-clean binary, ship the resulting file, and load it at startup:

```bash
rmg-baseline-generator --self --output myapp.rmgbaseline
```

```cpp
std::vector<std::byte> baselineBytes = /* read myapp.rmgbaseline from disk */;
guardian->loadBaseline(rmg::core::ByteView(baselineBytes.data(), baselineBytes.size()));
```

## Next Steps

- Browse [`examples/`](../../examples/) for five complete, runnable scenarios.
- Read [docs/architecture/OVERVIEW.md](../architecture/OVERVIEW.md) to understand how the pieces fit together.
- See [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) for advanced integration patterns.
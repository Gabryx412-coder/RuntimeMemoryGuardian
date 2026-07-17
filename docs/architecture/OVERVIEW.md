# Architecture Overview

Runtime Memory Guardian is organized in strict, unidirectional layers. Each layer depends only on the layers beneath it, never sideways or upward:
rmg::api        (RuntimeMemoryGuardian facade)
↓
rmg::process    (ProcessMonitor orchestration)
↓
rmg::integrity  rmg::hooks  rmg::modules   (domain logic, purely defensive)
↓
rmg::memory     (scanning, snapshotting, diffing)
↓
rmg::platform   (OS abstraction: IPlatformTraits)
↓
rmg::core + rmg::utils   (shared foundations, zero external dependencies)

## Why This Layering?

- **Testability.** Every layer above `rmg::platform` depends on the `IPlatformTraits` *interface*, not a concrete OS backend. This lets `MemoryScanner`, `IntegrityChecker`, `HookDetector`, and `ModuleMonitor` all be unit-tested against an in-memory mock backend, with no real process required (see `tests/unit/`).
- **Single seam for platform code.** Exactly two files know about native OS types outside of `src/platform/`: none. Every `#include <windows.h>` or POSIX header lives inside `src/platform/{windows,linux}/`.
- **Composability over inheritance.** `HookDetector` is a thin facade composing `InlineHookDetector`, `IatHookDetector`, and `EatHookDetector`; `ProcessMonitor` composes `IntegrityChecker`, `HookDetector`, and `ModuleMonitor`. Each piece remains independently usable.

## Key Types by Layer

| Layer | Key Types |
|-------|-----------|
| `rmg::core` | `Error`, `Result<T>`, `Logger`, `Signal<Args...>` |
| `rmg::platform` | `IPlatformTraits`, `ProcessHandle`, `MemoryRegion` |
| `rmg::memory` | `MemoryScanner`, `MemorySnapshot`, `MemoryDiff` |
| `rmg::integrity` | `IHashProvider`, `IntegrityBaseline`, `IntegrityChecker` |
| `rmg::hooks` | `HookDetector`, `InlineHookDetector`, `IatHookDetector`, `EatHookDetector` |
| `rmg::modules` | `ModuleEnumerator`, `ModuleMonitor`, `ModuleInfo` |
| `rmg::process` | `ProcessMonitor`, `MonitorConfig`, `MonitorEvent` |
| `rmg::api` | `RuntimeMemoryGuardian` |

## Error Handling Philosophy

RMG never throws exceptions across its public API. Every fallible operation returns `rmg::core::Result<T>` (an alias for `std::expected<T, Error>`). This is a deliberate choice for a library meant to run in the hot paths of security-sensitive host applications: callers can inspect `.has_value()`/`.error()` without needing to reason about exception safety at every call site.

See the per-subsystem documents in this directory for deeper dives into each layer.
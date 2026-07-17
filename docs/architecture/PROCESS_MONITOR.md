# Process Monitor Architecture

## Responsibility

`rmg::process::ProcessMonitor` is the highest-level orchestration component, wiring together `IntegrityChecker`, `HookDetector`, and `ModuleMonitor` into a single continuous or on-demand monitoring cycle.

## Usage Modes

- **Continuous**: `start()` spawns a background `std::thread` that calls `runOnce()` every `MonitorConfig::pollInterval` until `stop()` is called. Start/stop are idempotent (calling `start()` twice is a no-op; `stop()` is always safe to call).
- **On-demand**: `runOnce()` performs exactly one cycle synchronously on the calling thread — useful for callers who want to drive scheduling from their own event loop rather than accepting RMG's background thread.

## Unified Event Model

Every subsystem's findings are normalized into one `MonitorEvent` type (`MonitorEventType`: `IntegrityViolation`, `HookDetected`, `ModuleLoaded`, `ModuleUnloaded`) and delivered through a single `rmg::core::Signal<const MonitorEvent&> onEvent`. This means a consumer only needs one subscription point regardless of how many detection subsystems are enabled via `MonitorConfig`.

## Concurrency Notes

- `onEvent.emit()` is called synchronously on whichever thread performed the cycle (the background thread during `start()`, or the caller's thread during an explicit `runOnce()`). Listeners must not assume a specific thread.
- `ModuleMonitor`'s internal state is not thread-safe against concurrent `poll()` calls; calling `runOnce()` manually while `start()`'s background loop is also active is discouraged without external synchronization.
- `ProcessMonitor`'s destructor calls `stop()`, ensuring the background thread is always joined before the object is destroyed — no dangling thread references are possible.

## Baseline Ownership

`ProcessMonitor` owns the currently installed `IntegrityBaseline` (set via `setBaseline()`) and exposes it read-only via `baseline()`. This single-ownership design means the `RuntimeMemoryGuardian` facade's `checkIntegrity()` re-uses the exact same baseline `ProcessMonitor`'s own cycles verify against, rather than maintaining a second, potentially inconsistent copy.
# Integration Guide

## Choosing an Entry Point

Most applications should use the `rmg::api::RuntimeMemoryGuardian` facade — it wires together every subsystem with sensible defaults. Use the lower-level components (`rmg::memory`, `rmg::integrity`, `rmg::hooks`, `rmg::modules`, `rmg::process`) directly only if you need:

- A custom `IHashProvider` implementation.
- Fine-grained control over which regions are scanned.
- To share a single `MemoryScanner`/`IPlatformTraits` instance across multiple custom components.

## Monitoring an External Process

```cpp
auto guardian = rmg::api::RuntimeMemoryGuardian::createForProcess(targetPid);
```

This requires sufficient OS privileges to open the target process for memory reading (`PROCESS_VM_READ` on Windows; read access to `/proc/[pid]/mem` on Linux, which typically requires `CAP_SYS_PTRACE` or running as the same user/root).

## Choosing a Hash Provider

```cpp
auto guardian = rmg::api::RuntimeMemoryGuardian::createForSelf(
    std::make_unique<rmg::integrity::Sha256HashProvider>());
```

Use `Sha256HashProvider` (the default) for any integrity guarantee that matters. `Crc32HashProvider` is available only for cheap, high-frequency heuristic change detection where cryptographic strength is not required — never as the sole basis of a security-critical check.

## Handling MonitorEvents Responsibly

`ProcessMonitor::onEvent` fires synchronously on the polling thread. Keep listeners fast and non-blocking; if a response requires significant work (writing to a remote log, sending an alert), hand off to a separate thread or queue rather than doing it inline.

```cpp
guardian->monitor().onEvent.connect([&alertQueue](const rmg::process::MonitorEvent& event) {
    alertQueue.push(event); // Fast, non-blocking; a separate worker drains this queue.
});
```

## Combining with Your Own Detection Logic

Every `rmg::hooks`/`rmg::integrity`/`rmg::modules` component takes an `IPlatformTraits` or `MemoryScanner` reference rather than owning one internally. This lets you compose RMG's building blocks with your own custom checks, sharing the same underlying scanner instance for efficiency:

```cpp
auto platformTraits = rmg::platform::createPlatformTraits();
rmg::memory::MemoryScanner scanner(*platformTraits);

rmg::hooks::HookDetector hookDetector(scanner);
MyCustomDetector customDetector(scanner); // Your own class, same scanner.
```

## Threading Considerations

`ProcessMonitor::start()`/`stop()` manage exactly one background thread. If your application already has its own scheduling infrastructure (a task scheduler, an existing event loop), prefer calling `runOnce()` from within that infrastructure instead of using `start()`, to avoid running two independent scheduling mechanisms side by side.
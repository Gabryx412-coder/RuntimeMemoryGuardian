// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/modules/module_monitor.hpp
//
// Maintains a point-in-time record of a process' loaded modules and, upon
// each poll() call, diffs the current module list against the previous one
// to detect modules that have been loaded or unloaded since the last check.
// This is the mechanism that lets ProcessMonitor report "unexpected module
// loaded" events — a common indicator of runtime code injection.
// ==============================================================================

#pragma once

#include <rmg/core/error.hpp>
#include <rmg/core/event.hpp>
#include <rmg/modules/module_enumerator.hpp>
#include <rmg/platform/process_handle.hpp>

#include <unordered_map>
#include <vector>

namespace rmg::modules {

/// @brief Tracks the set of loaded modules for a target process over time,
///        emitting events when modules are loaded or unloaded.
///
/// ModuleMonitor is deliberately synchronous/pull-based (poll()) rather
/// than owning a background thread itself: this keeps its concurrency
/// model simple and lets the owning component (rmg::process::ProcessMonitor)
/// decide the polling cadence and threading model.
class ModuleMonitor final {
public:
    explicit ModuleMonitor(const ModuleEnumerator& enumerator) noexcept
        : enumerator_(&enumerator) {}

    /// @brief Emitted once per newly-detected module, on the thread that
    ///        calls poll().
    rmg::core::Signal<const ModuleInfo&> onModuleLoaded;

    /// @brief Emitted once per module no longer present, on the thread that
    ///        calls poll(). Only the last-known ModuleInfo is available
    ///        (the module's current, post-unload state cannot be read).
    rmg::core::Signal<const ModuleInfo&> onModuleUnloaded;

    /// @brief Re-enumerates the target process' modules and emits
    ///        onModuleLoaded/onModuleUnloaded for any differences versus
    ///        the previous poll() (or versus an empty baseline, on the
    ///        first call).
    [[nodiscard]] rmg::core::Result<void> poll(const rmg::platform::ProcessHandle& handle);

    /// @brief The module set as of the most recent successful poll() call.
    [[nodiscard]] const std::vector<ModuleInfo>& currentModules() const noexcept {
        return currentModules_;
    }

private:
    const ModuleEnumerator* enumerator_;
    std::vector<ModuleInfo> currentModules_;
};

} // namespace rmg::modules
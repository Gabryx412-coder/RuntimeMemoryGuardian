// ==============================================================================
// Runtime Memory Guardian
// File: src/modules/module_monitor.cpp
// ==============================================================================

#include <rmg/modules/module_monitor.hpp>

#include <algorithm>
#include <unordered_set>

namespace rmg::modules {

rmg::core::Result<void> ModuleMonitor::poll(const rmg::platform::ProcessHandle& handle) {
    auto latestModules = enumerator_->enumerate(handle);
    if (!latestModules) {
        return std::unexpected(latestModules.error());
    }

    // Index the previous and new module sets by base address, which is a
    // stable identity for "the same loaded module instance" within a single
    // process lifetime (a reload at a different address is correctly
    // treated as unload-then-load).
    std::unordered_set<rmg::core::Address> previousAddresses;
    previousAddresses.reserve(currentModules_.size());
    for (const ModuleInfo& module : currentModules_) {
        previousAddresses.insert(module.baseAddress);
    }

    std::unordered_set<rmg::core::Address> latestAddresses;
    latestAddresses.reserve(latestModules->size());
    for (const ModuleInfo& module : *latestModules) {
        latestAddresses.insert(module.baseAddress);
    }

    // Detect newly loaded modules.
    for (const ModuleInfo& module : *latestModules) {
        if (!previousAddresses.contains(module.baseAddress)) {
            onModuleLoaded.emit(module);
        }
    }

    // Detect unloaded modules (present before, absent now).
    for (const ModuleInfo& module : currentModules_) {
        if (!latestAddresses.contains(module.baseAddress)) {
            onModuleUnloaded.emit(module);
        }
    }

    currentModules_ = std::move(*latestModules);
    return {};
}

} // namespace rmg::modules
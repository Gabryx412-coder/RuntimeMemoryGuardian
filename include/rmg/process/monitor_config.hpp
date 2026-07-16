// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/process/monitor_config.hpp
//
// Centralized configuration controlling ProcessMonitor's polling behavior
// and which detection subsystems are active during each monitoring cycle.
// ==============================================================================

#pragma once

#include <chrono>

namespace rmg::process {

/// @brief Configuration governing a single ProcessMonitor instance.
///
/// All fields have sensible, conservative defaults so that
/// `MonitorConfig{}` alone is a reasonable starting point for a typical
/// self-integrity monitoring scenario.
struct MonitorConfig {
    /// @brief Interval between successive monitoring cycles when running in
    ///        continuous mode (ProcessMonitor::start()).
    std::chrono::milliseconds pollInterval = std::chrono::milliseconds(1000);

    /// @brief Whether each cycle performs an IntegrityChecker::verify()
    ///        pass against the configured baseline.
    bool enableIntegrityCheck = true;

    /// @brief Whether each cycle performs a HookDetector::scanAll() pass
    ///        against monitored modules.
    bool enableHookDetection = true;

    /// @brief Whether each cycle performs a ModuleMonitor::poll() pass to
    ///        detect newly loaded/unloaded modules.
    bool enableModuleMonitoring = true;

    /// @brief If true, a detected integrity violation or hook finding also
    ///        triggers an immediate module re-scan on the same cycle,
    ///        rather than waiting for the next scheduled poll. Useful for
    ///        gathering more context around a suspected compromise as soon
    ///        as it's observed.
    bool rescanModulesOnAnomaly = true;
};

} // namespace rmg::process
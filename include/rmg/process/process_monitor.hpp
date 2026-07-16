// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/process/process_monitor.hpp
//
// The highest-level orchestration component in RMG's domain layer. Ties
// together IntegrityChecker, HookDetector, and ModuleMonitor into a single
// continuous (or on-demand) monitoring loop, exposing every observation
// through one unified rmg::core::Signal<const MonitorEvent&>.
//
// ProcessMonitor supports two usage modes:
//   - Continuous: start() spawns a background thread that polls at
//     config.pollInterval until stop() is called.
//   - On-demand: runOnce() performs a single monitoring cycle synchronously
//     on the calling thread, for callers who want full control over
//     scheduling (e.g. driving it from their own event loop).
// ==============================================================================

#pragma once

#include <atomic>
#include <optional>
#include <thread>
#include <vector>

#include <rmg/core/error.hpp>
#include <rmg/core/event.hpp>
#include <rmg/hooks/hook_detector.hpp>
#include <rmg/integrity/integrity_baseline.hpp>
#include <rmg/integrity/integrity_checker.hpp>
#include <rmg/memory/memory_scanner.hpp>
#include <rmg/modules/module_enumerator.hpp>
#include <rmg/modules/module_monitor.hpp>
#include <rmg/platform/platform_traits.hpp>
#include <rmg/platform/process_handle.hpp>
#include <rmg/process/monitor_config.hpp>
#include <rmg/process/monitor_event.hpp>

namespace rmg::process {

/// @brief Orchestrates continuous or on-demand defensive monitoring of a
///        single target process.
///
/// ProcessMonitor does not own the ProcessHandle it monitors — the handle
/// is supplied at construction and must remain valid for the monitor's
/// entire lifetime (including across start()/stop() cycles).
class ProcessMonitor final {
public:
    /// @param handle          The process to monitor. Must outlive this
    ///                        object.
    /// @param platformTraits  Platform backend used to construct the
    ///                        internal MemoryScanner/ModuleEnumerator. Must
    ///                        outlive this object.
    /// @param hashProvider    Hash algorithm used for integrity
    ///                        verification; must be compatible with any
    ///                        baseline later passed to setBaseline(). Must
    ///                        outlive this object.
    /// @param config          Initial monitoring configuration.
    ProcessMonitor(const rmg::platform::ProcessHandle& handle,
                   const rmg::platform::IPlatformTraits& platformTraits,
                   const rmg::integrity::IHashProvider& hashProvider,
                   MonitorConfig config = {});

    ~ProcessMonitor();

    ProcessMonitor(const ProcessMonitor&) = delete;
    ProcessMonitor& operator=(const ProcessMonitor&) = delete;
    ProcessMonitor(ProcessMonitor&&) = delete;
    ProcessMonitor& operator=(ProcessMonitor&&) = delete;

    /// @brief Emitted for every observation produced during a monitoring
    ///        cycle, whether triggered by start()'s background loop or by
    ///        an explicit runOnce() call. Delivered synchronously on
    ///        whichever thread performed the cycle.
    rmg::core::Signal<const MonitorEvent&> onEvent;

    /// @brief Installs the integrity baseline to verify against on future
    ///        cycles. Replaces any previously set baseline. May be called
    ///        before or during monitoring.
    void setBaseline(rmg::integrity::IntegrityBaseline baseline);

    /// @brief Starts a background thread that calls runOnce() every
    ///        config.pollInterval until stop() is called. No-op if already
    ///        running.
    void start();

    /// @brief Signals the background thread (if running) to stop and joins
    ///        it. Safe to call even if start() was never called, or if
    ///        already stopped.
    void stop();

    /// @brief True if the background monitoring thread is currently active.
    [[nodiscard]] bool isRunning() const noexcept { return running_.load(std::memory_order_acquire); }

    /// @brief Performs exactly one monitoring cycle synchronously on the
    ///        calling thread: optionally verifies integrity, scans for
    ///        hooks, and polls for module changes, emitting onEvent for
    ///        every anomaly found.
    ///
    /// Safe to call while start() is also running in the background,
    /// though doing so concurrently on the same ProcessMonitor from
    /// multiple threads without external synchronization is not
    /// recommended, as ModuleMonitor's internal state is not itself
    /// thread-safe against concurrent poll() calls.
    [[nodiscard]] rmg::core::Result<void> runOnce();

    /// @brief The current monitoring configuration.
    [[nodiscard]] const MonitorConfig& config() const noexcept { return config_; }

    /// @brief Updates the monitoring configuration. Takes effect starting
    ///        from the next cycle.
    void setConfig(MonitorConfig config) noexcept { config_ = config; }

private:
    void backgroundLoop();
    void runIntegrityCheckCycle();
    void runHookDetectionCycle();
    void runModuleMonitoringCycle();

    const rmg::platform::ProcessHandle* handle_;
    const rmg::platform::IPlatformTraits* platformTraits_;
    const rmg::integrity::IHashProvider* hashProvider_;
    MonitorConfig config_;

    rmg::memory::MemoryScanner scanner_;
    rmg::modules::ModuleEnumerator moduleEnumerator_;
    rmg::modules::ModuleMonitor moduleMonitor_;
    rmg::hooks::HookDetector hookDetector_;
    rmg::integrity::IntegrityChecker integrityChecker_;

    std::optional<rmg::integrity::IntegrityBaseline> baseline_;

    std::thread workerThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
};

} // namespace rmg::process
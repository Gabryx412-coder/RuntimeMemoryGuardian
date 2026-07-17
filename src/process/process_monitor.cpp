// ==============================================================================
// Runtime Memory Guardian
// File: src/process/process_monitor.cpp
// ==============================================================================

#include <rmg/core/logger.hpp>
#include <rmg/process/process_monitor.hpp>

namespace rmg::process {

ProcessMonitor::ProcessMonitor(const rmg::platform::ProcessHandle& handle,
                               const rmg::platform::IPlatformTraits& platformTraits,
                               const rmg::integrity::IHashProvider& hashProvider,
                               MonitorConfig config)
    : handle_(&handle), platformTraits_(&platformTraits), hashProvider_(&hashProvider),
      config_(config), scanner_(platformTraits), moduleEnumerator_(platformTraits),
      moduleMonitor_(moduleEnumerator_), hookDetector_(scanner_),
      integrityChecker_(scanner_, hashProvider) {}

ProcessMonitor::~ProcessMonitor() {
    stop();
}

void ProcessMonitor::setBaseline(rmg::integrity::IntegrityBaseline baseline) {
    baseline_ = std::move(baseline);
}

void ProcessMonitor::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return; // Already running.
    }

    stopRequested_.store(false, std::memory_order_release);
    workerThread_ = std::thread(&ProcessMonitor::backgroundLoop, this);
}

void ProcessMonitor::stop() {
    stopRequested_.store(true, std::memory_order_release);
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    running_.store(false, std::memory_order_release);
}

void ProcessMonitor::backgroundLoop() {
    while (!stopRequested_.load(std::memory_order_acquire)) {
        auto result = runOnce();
        if (!result) {
            RMG_LOG_ERROR("ProcessMonitor::backgroundLoop: cycle failed: " +
                          result.error().toDiagnosticString());
        }
        std::this_thread::sleep_for(config_.pollInterval);
    }
}

rmg::core::Result<void> ProcessMonitor::runOnce() {
    if (config_.enableIntegrityCheck && baseline_.has_value()) {
        runIntegrityCheckCycle();
    }

    if (config_.enableHookDetection) {
        runHookDetectionCycle();
    }

    if (config_.enableModuleMonitoring) {
        runModuleMonitoringCycle();
    }

    return {};
}

void ProcessMonitor::runIntegrityCheckCycle() {
    auto report = integrityChecker_.verify(*handle_, *baseline_);
    if (!report) {
        RMG_LOG_WARNING("ProcessMonitor: integrity verification failed: " +
                        report.error().toDiagnosticString());
        return;
    }

    for (const rmg::integrity::TamperedSection& tampered : report->tamperedSections) {
        MonitorEvent event;
        event.type = MonitorEventType::IntegrityViolation;
        event.timestamp = std::chrono::system_clock::now();
        event.description = "integrity violation in section '" + tampered.section.name +
                            "' of module '" + tampered.section.ownerModule + "'";
        event.tamperedSection = tampered;
        onEvent.emit(event);
    }
}

void ProcessMonitor::runHookDetectionCycle() {
    auto currentModules = moduleEnumerator_.enumerate(*handle_);
    if (!currentModules) {
        RMG_LOG_WARNING("ProcessMonitor: module enumeration for hook scan failed: " +
                        currentModules.error().toDiagnosticString());
        return;
    }

    for (const rmg::modules::ModuleInfo& module : *currentModules) {
        auto findings = hookDetector_.scanAll(*handle_, module, *currentModules);
        if (!findings) {
            RMG_LOG_WARNING("ProcessMonitor: hook scan failed for module '" + module.name +
                            "': " + findings.error().toDiagnosticString());
            continue;
        }

        for (const rmg::hooks::HookFinding& finding : *findings) {
            MonitorEvent event;
            event.type = MonitorEventType::HookDetected;
            event.timestamp = std::chrono::system_clock::now();
            event.description = finding.description;
            event.hookFinding = finding;
            onEvent.emit(event);
        }
    }
}

void ProcessMonitor::runModuleMonitoringCycle() {
    // Wire ModuleMonitor's signals to onEvent for exactly this poll() call,
    // then disconnect afterwards so listeners are not duplicated across
    // cycles (ModuleMonitor is a long-lived member reused every cycle).
    rmg::core::Connection loadedConnection =
        moduleMonitor_.onModuleLoaded.connect([this](const rmg::modules::ModuleInfo& module) {
            MonitorEvent event;
            event.type = MonitorEventType::ModuleLoaded;
            event.timestamp = std::chrono::system_clock::now();
            event.description = "module loaded: " + module.name;
            event.moduleInfo = module;
            onEvent.emit(event);
        });

    rmg::core::Connection unloadedConnection =
        moduleMonitor_.onModuleUnloaded.connect([this](const rmg::modules::ModuleInfo& module) {
            MonitorEvent event;
            event.type = MonitorEventType::ModuleUnloaded;
            event.timestamp = std::chrono::system_clock::now();
            event.description = "module unloaded: " + module.name;
            event.moduleInfo = module;
            onEvent.emit(event);
        });

    auto result = moduleMonitor_.poll(*handle_);
    if (!result) {
        RMG_LOG_WARNING("ProcessMonitor: module monitoring poll failed: " +
                        result.error().toDiagnosticString());
    }

    moduleMonitor_.onModuleLoaded.disconnect(loadedConnection);
    moduleMonitor_.onModuleUnloaded.disconnect(unloadedConnection);
}

} // namespace rmg::process
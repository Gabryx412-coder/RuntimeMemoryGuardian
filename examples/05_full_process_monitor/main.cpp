// ==============================================================================
// Runtime Memory Guardian - Example 05: Full Process Monitor
//
// Demonstrates the complete, high-level monitoring workflow using the
// RuntimeMemoryGuardian facade's ProcessMonitor: establish a baseline,
// subscribe to every category of MonitorEvent, then run continuous
// background monitoring for a short demonstration period before shutting
// down cleanly.
//
// In a real application, start() would typically be called once during
// startup (after establishBaseline()/loadBaseline()) and stop() during
// graceful shutdown, with onEvent driving whatever response the
// application deems appropriate (logging, alerting, terminating, etc.).
// ==============================================================================

#include <rmg/rmg.hpp>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>

namespace {

void handleEvent(const rmg::process::MonitorEvent& event) {
    switch (event.type) {
        case rmg::process::MonitorEventType::IntegrityViolation:
            std::printf("[!] INTEGRITY VIOLATION: %s\n", event.description.c_str());
            break;
        case rmg::process::MonitorEventType::HookDetected:
            std::printf("[!] HOOK DETECTED: %s\n", event.description.c_str());
            break;
        case rmg::process::MonitorEventType::ModuleLoaded:
            std::printf("[+] MODULE LOADED: %s\n", event.description.c_str());
            break;
        case rmg::process::MonitorEventType::ModuleUnloaded:
            std::printf("[-] MODULE UNLOADED: %s\n", event.description.c_str());
            break;
    }
}

} // namespace

int main() {
    std::printf("Runtime Memory Guardian - Example 05: Full Process Monitor\n\n");

    auto guardianResult = rmg::api::RuntimeMemoryGuardian::createForSelf();
    if (!guardianResult) {
        std::fprintf(stderr, "Failed to create guardian: %s\n",
                     guardianResult.error().toDiagnosticString().c_str());
        return 1;
    }
    auto& guardian = *guardianResult;

    std::printf("Establishing baseline...\n");
    if (auto result = guardian.establishBaseline(); !result) {
        std::fprintf(stderr, "Failed to establish baseline: %s\n",
                     result.error().toDiagnosticString().c_str());
        return 1;
    }

    rmg::process::MonitorConfig config;
    config.pollInterval = std::chrono::milliseconds(500);
    config.enableIntegrityCheck = true;
    config.enableHookDetection = true;
    config.enableModuleMonitoring = true;
    guardian.monitor().setConfig(config);

    std::atomic<int> eventCount{0};
    guardian.monitor().onEvent.connect([&eventCount](const rmg::process::MonitorEvent& event) {
        ++eventCount;
        handleEvent(event);
    });

    std::printf("Starting continuous monitoring (polling every %lldms)...\n",
                static_cast<long long>(config.pollInterval.count()));
    guardian.monitor().start();

    constexpr auto DEMO_DURATION = std::chrono::seconds(3);
    std::printf("Monitoring for %lld second(s) as a demonstration...\n",
                static_cast<long long>(DEMO_DURATION.count()));
    std::this_thread::sleep_for(DEMO_DURATION);

    std::printf("Stopping monitor...\n");
    guardian.monitor().stop();

    std::printf("\nDemonstration complete. Total events observed: %d\n", eventCount.load());
    std::printf("(Zero events is the expected, healthy outcome for this unmodified example.)\n");

    return 0;
}
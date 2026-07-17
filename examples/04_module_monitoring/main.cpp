// ==============================================================================
// Runtime Memory Guardian - Example 04: Module Monitoring
//
// Demonstrates using ModuleMonitor directly to observe modules being loaded
// or unloaded from the current process over time. Since this example
// process does not dynamically load libraries itself, the first poll()
// reports every currently-loaded module as "newly loaded" (there being no
// prior state to compare against); a second poll shortly after shows the
// (expected, stable) module set producing no further events.
// ==============================================================================

#include <rmg/rmg.hpp>

#include <cstdio>
#include <thread>

int main() {
    std::printf("Runtime Memory Guardian - Example 04: Module Monitoring\n\n");

    auto platformTraits = rmg::platform::createPlatformTraits();
    auto handleResult = rmg::platform::ProcessHandle::openSelf();
    if (!handleResult) {
        std::fprintf(stderr, "Failed to open self handle: %s\n",
                     handleResult.error().toDiagnosticString().c_str());
        return 1;
    }

    rmg::modules::ModuleEnumerator enumerator(*platformTraits);
    rmg::modules::ModuleMonitor monitor(enumerator);

    monitor.onModuleLoaded.connect([](const rmg::modules::ModuleInfo& module) {
        std::printf("[LOADED]   %-30s base=0x%016zx size=%zu\n", module.name.c_str(),
                    static_cast<std::size_t>(module.baseAddress), module.size);
    });

    monitor.onModuleUnloaded.connect([](const rmg::modules::ModuleInfo& module) {
        std::printf("[UNLOADED] %-30s (was at 0x%016zx)\n", module.name.c_str(),
                    static_cast<std::size_t>(module.baseAddress));
    });

    std::printf("--- First poll (baseline: reports every currently loaded module) ---\n");
    auto firstPoll = monitor.poll(*handleResult);
    if (!firstPoll) {
        std::fprintf(stderr, "First poll failed: %s\n",
                     firstPoll.error().toDiagnosticString().c_str());
        return 1;
    }
    std::printf("Total modules tracked: %zu\n\n", monitor.currentModules().size());

    std::printf("--- Second poll (expected: no changes) ---\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto secondPoll = monitor.poll(*handleResult);
    if (!secondPoll) {
        std::fprintf(stderr, "Second poll failed: %s\n",
                     secondPoll.error().toDiagnosticString().c_str());
        return 1;
    }
    std::printf("(No output above this line means no modules changed, as expected.)\n");

    return 0;
}
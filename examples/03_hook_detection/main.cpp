// ==============================================================================
// Runtime Memory Guardian - Example 03: Hook Detection
//
// Demonstrates running HookDetector across every module loaded in the
// current process, reporting any suspected inline/IAT/EAT hooks found.
//
// Note: on a clean system, this example is expected to report zero
// findings. Instrumented environments (sanitizers, profilers, some
// security/monitoring tools) may legitimately install inline hooks into
// common library functions, which would surface here as findings — this is
// a expected characteristic of heuristic, pattern-based defensive scanning,
// not a bug in RMG.
// ==============================================================================

#include <cstdio>

#include <rmg/rmg.hpp>

int main() {
    std::printf("Runtime Memory Guardian - Example 03: Hook Detection\n\n");

    auto guardianResult = rmg::api::RuntimeMemoryGuardian::createForSelf();
    if (!guardianResult) {
        std::fprintf(stderr, "Failed to create guardian: %s\n",
                     guardianResult.error().toDiagnosticString().c_str());
        return 1;
    }

    std::printf("Scanning all loaded modules for suspected hooks...\n");
    auto findingsResult = guardianResult->detectHooks();
    if (!findingsResult) {
        std::fprintf(stderr, "Hook detection failed: %s\n",
                     findingsResult.error().toDiagnosticString().c_str());
        return 1;
    }

    if (findingsResult->empty()) {
        std::printf("No suspected hooks found.\n");
        return 0;
    }

    std::printf("Found %zu suspected hook(s):\n\n", findingsResult->size());
    for (const auto& finding : *findingsResult) {
        std::printf("  Type:       %s\n", std::string(rmg::hooks::toString(finding.type)).c_str());
        std::printf("  Location:   0x%zx\n", static_cast<std::size_t>(finding.location));
        std::printf("  Module:     %s\n", finding.moduleName.c_str());
        if (!finding.targetSymbol.empty()) {
            std::printf("  Symbol:     %s\n", finding.targetSymbol.c_str());
        }
        std::printf("  Details:    %s\n\n", finding.description.c_str());
    }

    return 0;
}
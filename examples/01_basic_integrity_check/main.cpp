// ==============================================================================
// Runtime Memory Guardian - Example 01: Basic Integrity Check
//
// Demonstrates the simplest possible use of RMG: monitor the CURRENT
// process's own code sections for tampering. This is the canonical
// "self-integrity" scenario — an application embeds RMG to detect whether
// its own executable code has been patched at runtime (e.g. by a debugger,
// an injected DLL, or a malicious hook).
//
// Flow:
//   1. Create a guardian for the current process.
//   2. Establish a baseline (hash every loaded module's code sections).
//   3. Immediately verify against that baseline (expected: clean).
// ==============================================================================

#include <rmg/rmg.hpp>

#include <cstdio>

namespace {

void printReport(const rmg::integrity::IntegrityReport& report) {
    if (report.isValid) {
        std::printf("[OK] Integrity check passed: no tampering detected.\n");
    } else {
        std::printf("[ALERT] Integrity violation detected in %zu section(s):\n",
                    report.tamperedSections.size());
        for (const auto& tampered : report.tamperedSections) {
            std::printf("  - module '%s', section '%s' at 0x%zx\n",
                        tampered.section.ownerModule.c_str(), tampered.section.name.c_str(),
                        static_cast<std::size_t>(tampered.section.baseAddress));
        }
    }

    if (!report.unreadableSections.empty()) {
        std::printf("[WARN] %zu section(s) could not be read during verification.\n",
                    report.unreadableSections.size());
    }
}

} // namespace

int main() {
    std::printf("Runtime Memory Guardian v%s - Example 01: Basic Integrity Check\n",
                std::string(rmg::VERSION_STRING).c_str());

    auto guardianResult = rmg::api::RuntimeMemoryGuardian::createForSelf();
    if (!guardianResult) {
        std::fprintf(stderr, "Failed to create guardian: %s\n",
                     guardianResult.error().toDiagnosticString().c_str());
        return 1;
    }
    auto& guardian = *guardianResult;

    std::printf("Establishing integrity baseline for the current process...\n");
    auto baselineResult = guardian.establishBaseline();
    if (!baselineResult) {
        std::fprintf(stderr, "Failed to establish baseline: %s\n",
                     baselineResult.error().toDiagnosticString().c_str());
        return 1;
    }
    std::printf("Baseline established.\n\n");

    std::printf("Verifying integrity...\n");
    auto reportResult = guardian.checkIntegrity();
    if (!reportResult) {
        std::fprintf(stderr, "Failed to verify integrity: %s\n",
                     reportResult.error().toDiagnosticString().c_str());
        return 1;
    }

    printReport(*reportResult);
    return reportResult->isValid ? 0 : 2;
}
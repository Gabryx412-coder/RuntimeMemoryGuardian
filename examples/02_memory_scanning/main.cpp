// ==============================================================================
// Runtime Memory Guardian - Example 02: Memory Scanning
//
// Demonstrates using MemoryScanner directly (bypassing the RuntimeMemoryGuardian
// facade) for finer-grained control: enumerating memory regions of the
// current process and applying filters (executable-only, W^X exclusion).
// ==============================================================================

#include <cstdio>

#include <rmg/rmg.hpp>

namespace {

void printRegionSummary(const rmg::memory::MemorySnapshot& snapshot) {
    std::printf("Captured %zu region(s):\n", snapshot.regions().size());

    std::size_t totalBytes = 0;
    for (const auto& snapshotRegion : snapshot.regions()) {
        const auto& region = snapshotRegion.region;
        std::printf("  0x%016zx  size=%-10zu prot=%-4s module=%s\n",
                    static_cast<std::size_t>(region.baseAddress),
                    region.size,
                    rmg::core::toString(region.protection).c_str(),
                    region.moduleName.empty() ? "<anonymous>" : region.moduleName.c_str());
        totalBytes += snapshotRegion.contents.size();
    }

    std::printf("Total captured bytes: %zu\n", totalBytes);
}

} // namespace

int main() {
    std::printf("Runtime Memory Guardian - Example 02: Memory Scanning\n\n");

    auto platformTraits = rmg::platform::createPlatformTraits();
    auto handleResult = rmg::platform::ProcessHandle::openSelf();
    if (!handleResult) {
        std::fprintf(stderr, "Failed to open self handle: %s\n",
                     handleResult.error().toDiagnosticString().c_str());
        return 1;
    }

    rmg::memory::MemoryScanner scanner(*platformTraits);

    std::printf("--- Scanning executable regions only ---\n");
    rmg::memory::ScanFilter executableFilter;
    executableFilter.executableOnly = true;

    auto executableSnapshot = scanner.scan(*handleResult, executableFilter);
    if (!executableSnapshot) {
        std::fprintf(stderr, "Executable scan failed: %s\n",
                     executableSnapshot.error().toDiagnosticString().c_str());
        return 1;
    }
    printRegionSummary(*executableSnapshot);

    std::printf("\n--- Scanning for suspicious writable+executable regions ---\n");
    rmg::memory::ScanFilter wxFilter;
    wxFilter.executableOnly = true;
    wxFilter.excludeWritableExecutable = false; // We WANT to see W^X violations here.

    auto allExecutable = scanner.scan(*handleResult, wxFilter);
    if (!allExecutable) {
        std::fprintf(stderr, "Scan failed: %s\n", allExecutable.error().toDiagnosticString().c_str());
        return 1;
    }

    bool foundSuspicious = false;
    for (const auto& snapshotRegion : allExecutable->regions()) {
        if (snapshotRegion.region.isWritable()) {
            std::printf("  [SUSPICIOUS] Writable+Executable region at 0x%zx (module: %s)\n",
                        static_cast<std::size_t>(snapshotRegion.region.baseAddress),
                        snapshotRegion.region.moduleName.c_str());
            foundSuspicious = true;
        }
    }

    if (!foundSuspicious) {
        std::printf("  None found (this is the expected, healthy state).\n");
    }

    return 0;
}
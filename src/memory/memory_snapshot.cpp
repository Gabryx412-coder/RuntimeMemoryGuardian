// ==============================================================================
// Runtime Memory Guardian
// File: src/memory/memory_snapshot.cpp
// ==============================================================================

#include <rmg/core/logger.hpp>
#include <rmg/memory/memory_snapshot.hpp>

namespace rmg::memory {

rmg::core::Result<MemorySnapshot>
MemorySnapshot::capture(const rmg::platform::ProcessHandle& handle,
                        std::span<const rmg::platform::MemoryRegion> regions,
                        const rmg::platform::IPlatformTraits& platformTraits) {
    MemorySnapshot snapshot;
    snapshot.capturedAt_ = std::chrono::system_clock::now();
    snapshot.regions_.reserve(regions.size());

    for (const rmg::platform::MemoryRegion& region : regions) {
        rmg::utils::ByteBuffer buffer(region.size);

        auto bytesRead =
            platformTraits.readMemory(handle, region.baseAddress, buffer.mutableView());
        if (!bytesRead) {
            RMG_LOG_DEBUG("MemorySnapshot::capture: skipping region at " +
                          std::to_string(region.baseAddress) + " (" +
                          bytesRead.error().toDiagnosticString() + ")");
            continue;
        }

        // A short read (e.g. straddling an unmapped guard page) is not
        // fatal: shrink the buffer to the bytes actually captured so
        // downstream consumers never see uninitialized trailing bytes.
        if (*bytesRead != buffer.size()) {
            buffer.resize(*bytesRead);
        }

        snapshot.regions_.push_back(SnapshotRegion{region, std::move(buffer)});
    }

    if (snapshot.regions_.empty() && !regions.empty()) {
        return rmg::core::fail<MemorySnapshot>(rmg::core::ErrorCode::MemoryAccessFailure,
                                               "failed to capture any of the " +
                                                   std::to_string(regions.size()) +
                                                   " requested regions");
    }

    return snapshot;
}

const SnapshotRegion*
MemorySnapshot::findRegionContaining(rmg::core::Address address) const noexcept {
    for (const SnapshotRegion& entry : regions_) {
        if (entry.region.contains(address)) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace rmg::memory
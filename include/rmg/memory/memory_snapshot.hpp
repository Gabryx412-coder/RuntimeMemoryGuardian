// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/memory/memory_snapshot.hpp
//
// Represents an immutable, point-in-time capture of the contents of one or
// more memory regions. A MemorySnapshot is the unit of comparison used both
// for integrity baselines (IntegrityBaseline wraps a MemorySnapshot taken at
// "known-good" time) and for change detection (MemoryDiff compares two
// snapshots taken at different times).
// ==============================================================================

#pragma once

#include <chrono>
#include <optional>
#include <vector>

#include <rmg/core/error.hpp>
#include <rmg/core/types.hpp>
#include <rmg/platform/memory_region.hpp>
#include <rmg/platform/platform_traits.hpp>
#include <rmg/platform/process_handle.hpp>
#include <rmg/utils/byte_buffer.hpp>

namespace rmg::memory {

/// @brief The captured contents of a single memory region at snapshot time.
struct SnapshotRegion {
    rmg::platform::MemoryRegion region;
    rmg::utils::ByteBuffer contents;
};

/// @brief An immutable capture of one or more memory regions' contents,
///        taken at a specific instant.
///
/// MemorySnapshot is deliberately non-copyable-by-default-use (it holds
/// potentially large byte buffers) but is movable; callers that need to
/// retain multiple snapshots simultaneously (e.g. for MemoryDiff) should
/// hold them in separate named variables rather than copying.
class MemorySnapshot final {
public:
    /// @brief Captures the current contents of @p regions from the process
    ///        referenced by @p handle, using @p platformTraits to perform
    ///        the underlying reads.
    ///
    /// @return A populated MemorySnapshot on success. Individual region
    ///         reads that fail (e.g. a page was unmapped between
    ///         enumeration and read) are skipped rather than failing the
    ///         entire capture; callers can compare
    ///         `regions().size()` against the input to detect this.
    [[nodiscard]] static rmg::core::Result<MemorySnapshot>
    capture(const rmg::platform::ProcessHandle& handle,
            std::span<const rmg::platform::MemoryRegion> regions,
            const rmg::platform::IPlatformTraits& platformTraits);

    MemorySnapshot(MemorySnapshot&&) = default;
    MemorySnapshot& operator=(MemorySnapshot&&) = default;
    MemorySnapshot(const MemorySnapshot&) = delete;
    MemorySnapshot& operator=(const MemorySnapshot&) = delete;

    /// @brief The instant at which this snapshot was captured.
    [[nodiscard]] std::chrono::system_clock::time_point capturedAt() const noexcept {
        return capturedAt_;
    }

    /// @brief All successfully captured regions, in enumeration order.
    [[nodiscard]] const std::vector<SnapshotRegion>& regions() const noexcept { return regions_; }

    /// @brief Finds the captured region containing @p address, if any.
    [[nodiscard]] const SnapshotRegion* findRegionContaining(rmg::core::Address address) const noexcept;

private:
    MemorySnapshot() = default;

    std::chrono::system_clock::time_point capturedAt_;
    std::vector<SnapshotRegion> regions_;
};

} // namespace rmg::memory
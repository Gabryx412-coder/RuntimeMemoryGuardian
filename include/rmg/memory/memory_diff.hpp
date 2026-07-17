// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/memory/memory_diff.hpp
//
// Compares two MemorySnapshot captures of the same (or overlapping) address
// ranges to detect unexpected memory modification — the core mechanism
// behind runtime integrity monitoring and change-based hook detection.
// ==============================================================================

#pragma once

#include <rmg/core/types.hpp>
#include <rmg/memory/memory_snapshot.hpp>

#include <vector>

namespace rmg::memory {

/// @brief Describes a single contiguous run of bytes that differs between
///        two snapshots.
struct ByteRangeChange {
    /// @brief Absolute address (in the target process) where the change
    ///        begins.
    rmg::core::Address address = 0;

    /// @brief Number of consecutive changed bytes starting at @c address.
    std::size_t length = 0;
};

/// @brief The full result of comparing two MemorySnapshot instances.
struct DiffResult {
    /// @brief Every contiguous changed byte range found, in ascending
    ///        address order.
    std::vector<ByteRangeChange> changedRanges;

    /// @brief True if the two snapshots covered exactly the same set of
    ///        regions. False indicates a region present in one snapshot was
    ///        missing from the other (e.g. a module was unloaded), which
    ///        callers may want to treat as noteworthy independent of byte
    ///        content changes.
    bool regionSetIdentical = true;

    /// @brief Convenience accessor: true if no byte-level changes were found
    ///        (regardless of regionSetIdentical).
    [[nodiscard]] bool hasChanges() const noexcept { return !changedRanges.empty(); }
};

/// @brief Stateless comparator between two MemorySnapshot instances.
class MemoryDiff final {
public:
    MemoryDiff() = delete;

    /// @brief Compares @p baseline against @p current, reporting every
    ///        contiguous byte range that differs.
    ///
    /// Comparison is performed per-region: a region is matched between the
    ///  two snapshots by base address. Regions present in only one snapshot
    /// are reflected via DiffResult::regionSetIdentical rather than as a
    /// byte-level change.
    [[nodiscard]] static DiffResult compare(const MemorySnapshot& baseline,
                                            const MemorySnapshot& current);
};

} // namespace rmg::memory
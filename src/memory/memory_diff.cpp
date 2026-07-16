// ==============================================================================
// Runtime Memory Guardian
// File: src/memory/memory_diff.cpp
// ==============================================================================

#include <rmg/memory/memory_diff.hpp>

#include <algorithm>

namespace rmg::memory {

namespace {

/// @brief Compares the contents of two SnapshotRegion instances that are
///        assumed to cover the same base address, appending any changed
///        byte ranges to @p outChanges.
void compareRegionContents(const SnapshotRegion& baseline,
                           const SnapshotRegion& current,
                           std::vector<ByteRangeChange>& outChanges) {
    const rmg::core::ByteView baselineBytes = baseline.contents.view();
    const rmg::core::ByteView currentBytes = current.contents.view();

    const std::size_t comparableLength = std::min(baselineBytes.size(), currentBytes.size());

    std::size_t i = 0;
    while (i < comparableLength) {
        if (baselineBytes[i] == currentBytes[i]) {
            ++i;
            continue;
        }

        // Found the start of a changed run; extend it as far as the
        // mismatch continues to produce a single contiguous ByteRangeChange
        // instead of one entry per byte.
        const std::size_t runStart = i;
        while (i < comparableLength && baselineBytes[i] != currentBytes[i]) {
            ++i;
        }

        ByteRangeChange change;
        change.address = baseline.region.baseAddress + static_cast<rmg::core::Address>(runStart);
        change.length = i - runStart;
        outChanges.push_back(change);
    }
}

} // namespace

DiffResult MemoryDiff::compare(const MemorySnapshot& baseline, const MemorySnapshot& current) {
    DiffResult result;

    for (const SnapshotRegion& baselineRegion : baseline.regions()) {
        const SnapshotRegion* currentRegion =
            current.findRegionContaining(baselineRegion.region.baseAddress);

        if (currentRegion == nullptr ||
            currentRegion->region.baseAddress != baselineRegion.region.baseAddress) {
            result.regionSetIdentical = false;
            continue;
        }

        compareRegionContents(baselineRegion, *currentRegion, result.changedRanges);
    }

    // Detect regions present in `current` but absent from `baseline`
    // (newly appeared mappings), which also signals a non-identical region set.
    for (const SnapshotRegion& currentRegion : current.regions()) {
        if (baseline.findRegionContaining(currentRegion.region.baseAddress) == nullptr) {
            result.regionSetIdentical = false;
        }
    }

    std::sort(result.changedRanges.begin(), result.changedRanges.end(),
              [](const ByteRangeChange& a, const ByteRangeChange& b) { return a.address < b.address; });

    return result;
}

} // namespace rmg::memory
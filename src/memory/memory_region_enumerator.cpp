// ==============================================================================
// Runtime Memory Guardian
// File: src/memory/memory_region_enumerator.cpp
// ==============================================================================

#include <rmg/memory/memory_region_enumerator.hpp>

#include <algorithm>
#include <iterator>

namespace rmg::memory {

rmg::core::Result<std::vector<rmg::platform::MemoryRegion>>
MemoryRegionEnumerator::enumerate(const rmg::platform::ProcessHandle& handle) const {
    return platformTraits_->enumerateRegions(handle);
}

rmg::core::Result<std::vector<rmg::platform::MemoryRegion>>
MemoryRegionEnumerator::enumerateExecutable(const rmg::platform::ProcessHandle& handle) const {
    auto allRegions = enumerate(handle);
    if (!allRegions) {
        return std::unexpected(allRegions.error());
    }

    std::vector<rmg::platform::MemoryRegion> executableRegions;
    std::copy_if(allRegions->begin(), allRegions->end(),
                 std::back_inserter(executableRegions),
                 [](const rmg::platform::MemoryRegion& region) {
                     return region.isCommitted && region.isExecutable();
                 });

    return executableRegions;
}

} // namespace rmg::memory
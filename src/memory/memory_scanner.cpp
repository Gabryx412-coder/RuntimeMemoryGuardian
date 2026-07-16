// ==============================================================================
// Runtime Memory Guardian
// File: src/memory/memory_scanner.cpp
// ==============================================================================

#include <rmg/memory/memory_scanner.hpp>

#include <algorithm>
#include <iterator>

#include <rmg/utils/string_utils.hpp>

namespace rmg::memory {

bool MemoryScanner::matchesFilter(const rmg::platform::MemoryRegion& region, const ScanFilter& filter) const {
    if (!region.isCommitted) {
        return false;
    }

    if (filter.executableOnly && !region.isExecutable()) {
        return false;
    }

    if (filter.excludeWritableExecutable && region.isExecutable() && region.isWritable()) {
        return false;
    }

    if (filter.moduleName.has_value()) {
        if (!rmg::utils::iequals(region.moduleName, *filter.moduleName)) {
            return false;
        }
    }

    return true;
}

rmg::core::Result<MemorySnapshot>
MemoryScanner::scan(const rmg::platform::ProcessHandle& handle, const ScanFilter& filter) const {
    auto allRegions = regionEnumerator_.enumerate(handle);
    if (!allRegions) {
        return std::unexpected(allRegions.error());
    }

    std::vector<rmg::platform::MemoryRegion> filtered;
    std::copy_if(allRegions->begin(), allRegions->end(), std::back_inserter(filtered),
                 [this, &filter](const rmg::platform::MemoryRegion& region) {
                     return matchesFilter(region, filter);
                 });

    return MemorySnapshot::capture(handle, filtered, *platformTraits_);
}

rmg::core::Result<rmg::utils::ByteBuffer>
MemoryScanner::read(const rmg::platform::ProcessHandle& handle, rmg::core::Address address, std::size_t size) const {
    rmg::utils::ByteBuffer buffer(size);

    auto bytesRead = platformTraits_->readMemory(handle, address, buffer.mutableView());
    if (!bytesRead) {
        return std::unexpected(bytesRead.error());
    }

    if (*bytesRead != size) {
        buffer.resize(*bytesRead);
    }

    return buffer;
}

} // namespace rmg::memory
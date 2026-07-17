// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/linux/linux_module_enumerator.cpp
//
// Implements detail::enumerateModulesLinux by collapsing consecutive
// /proc/[pid]/maps regions that share the same backing file path into a
// single NativeModuleInfo entry. A shared library typically appears as
// several adjacent mappings (one per ELF segment: r-x, r--, rw-), so this
// pass merges them into one logical module spanning
// [minBaseAddress, maxEndAddress).
// ==============================================================================

#include "linux_process_handle.hpp"

#include <algorithm>
#include <limits>
#include <unordered_map>

namespace rmg::platform::detail {

rmg::core::Result<std::vector<NativeModuleInfo>>
enumerateModulesLinux(const ProcessHandle& handle) {
    auto regions = enumerateRegionsLinux(handle);
    if (!regions) {
        return std::unexpected(regions.error());
    }

    // Group file-backed regions by their backing path, tracking the
    // smallest base address and largest end address seen per module so the
    // merged NativeModuleInfo spans the module's full image.
    struct Accumulator {
        rmg::core::Address minBase = std::numeric_limits<rmg::core::Address>::max();
        rmg::core::Address maxEnd = 0;
        std::string name;
    };

    std::unordered_map<std::string, Accumulator> byPath;
    // Preserve first-seen order for deterministic output.
    std::vector<std::string> orderedPaths;

    for (const MemoryRegion& region : *regions) {
        if (region.modulePath.empty()) {
            continue; // Anonymous mapping (heap, stack, JIT); not a module.
        }

        auto [it, inserted] = byPath.try_emplace(region.modulePath);
        if (inserted) {
            orderedPaths.push_back(region.modulePath);
            it->second.name = region.moduleName;
        }

        Accumulator& acc = it->second;
        acc.minBase = std::min(acc.minBase, region.baseAddress);
        acc.maxEnd = std::max(acc.maxEnd, region.endAddress());
    }

    std::vector<NativeModuleInfo> result;
    result.reserve(orderedPaths.size());

    for (const std::string& path : orderedPaths) {
        const Accumulator& acc = byPath.at(path);

        NativeModuleInfo info;
        info.path = path;
        info.name = acc.name;
        info.baseAddress = acc.minBase;
        info.size = static_cast<std::size_t>(acc.maxEnd - acc.minBase);

        result.push_back(std::move(info));
    }

    return result;
}

} // namespace rmg::platform::detail
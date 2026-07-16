// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/windows/win_memory_region_enumerator.cpp
//
// Implements detail::enumerateRegionsWindows by walking the target process'
// address space with successive VirtualQueryEx calls, resolving the backing
// module (if any) for each region via GetMappedFileNameA.
// ==============================================================================

#include "win_process_handle.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <psapi.h>
#include <windows.h>

namespace rmg::platform::detail {

namespace {

/// @brief Attempts to resolve the device-path-form backing file name for
///        the mapping containing @p address, returning an empty string if
///        the region is not backed by a file (anonymous memory).
[[nodiscard]] std::string tryResolveMappedFileName(HANDLE processHandle, LPVOID address) {
    char pathBuffer[MAX_PATH] = {};
    const DWORD length = ::GetMappedFileNameA(processHandle, address, pathBuffer, MAX_PATH);
    if (length == 0) {
        return {};
    }
    return std::string(pathBuffer, length);
}

/// @brief Extracts the file name component (e.g. "kernel32.dll") from a
///        full device-path-form module path.
[[nodiscard]] std::string extractFileName(const std::string& fullPath) {
    const std::size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash == std::string::npos) {
        return fullPath;
    }
    return fullPath.substr(lastSlash + 1);
}

} // namespace

rmg::core::Result<std::vector<MemoryRegion>>
enumerateRegionsWindows(const ProcessHandle& handle) {
    HANDLE winHandle = toWinHandle(handle.nativeHandle());

    std::vector<MemoryRegion> regions;
    auto address = reinterpret_cast<std::uint8_t*>(nullptr);

    MEMORY_BASIC_INFORMATION info{};
    while (::VirtualQueryEx(winHandle, address, &info, sizeof(info)) == sizeof(info)) {
        if (info.State == MEM_COMMIT || info.State == MEM_RESERVE) {
            MemoryRegion region;
            region.baseAddress = reinterpret_cast<rmg::core::Address>(info.BaseAddress);
            region.size = info.RegionSize;
            region.protection = toRmgProtection(static_cast<std::uint32_t>(info.Protect));
            region.isCommitted = (info.State == MEM_COMMIT);

            if (info.State == MEM_COMMIT) {
                const std::string mappedPath =
                    tryResolveMappedFileName(winHandle, info.BaseAddress);
                if (!mappedPath.empty()) {
                    region.modulePath = mappedPath;
                    region.moduleName = extractFileName(mappedPath);
                }
            }

            regions.push_back(std::move(region));
        }

        // Advance past the region we just inspected. Guard against a
        // zero-size region (should not normally happen) to avoid an
        // infinite loop.
        const std::uint8_t* nextAddress =
            reinterpret_cast<const std::uint8_t*>(info.BaseAddress) +
            (info.RegionSize != 0 ? info.RegionSize : 1);

        if (nextAddress <= address) {
            break;
        }
        address = const_cast<std::uint8_t*>(nextAddress);
    }

    return regions;
}

} // namespace rmg::platform::detail
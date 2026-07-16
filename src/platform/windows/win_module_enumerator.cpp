// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/windows/win_module_enumerator.cpp
//
// Implements detail::enumerateModulesWindows via EnumProcessModulesEx,
// GetModuleInformation, and GetModuleFileNameExA.
// ==============================================================================

#include "win_process_handle.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <psapi.h>
#include <windows.h>

#include <array>

namespace rmg::platform::detail {

namespace {

[[nodiscard]] std::string extractFileName(const std::string& fullPath) {
    const std::size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash == std::string::npos) {
        return fullPath;
    }
    return fullPath.substr(lastSlash + 1);
}

} // namespace

rmg::core::Result<std::vector<NativeModuleInfo>>
enumerateModulesWindows(const ProcessHandle& handle) {
    HANDLE winHandle = toWinHandle(handle.nativeHandle());

    // First pass: query the required buffer size.
    DWORD bytesNeeded = 0;
    if (::EnumProcessModulesEx(winHandle, nullptr, 0, &bytesNeeded, LIST_MODULES_ALL) == 0) {
        return rmg::core::fail<std::vector<NativeModuleInfo>>(
            rmg::core::ErrorCode::PlatformError,
            "EnumProcessModulesEx (size query) failed: " + lastErrorToString(::GetLastError()));
    }

    if (bytesNeeded == 0) {
        return std::vector<NativeModuleInfo>{};
    }

    std::vector<HMODULE> moduleHandles(bytesNeeded / sizeof(HMODULE));
    DWORD bytesReturned = 0;

    if (::EnumProcessModulesEx(
            winHandle,
            moduleHandles.data(),
            static_cast<DWORD>(moduleHandles.size() * sizeof(HMODULE)),
            &bytesReturned,
            LIST_MODULES_ALL) == 0) {
        return rmg::core::fail<std::vector<NativeModuleInfo>>(
            rmg::core::ErrorCode::PlatformError,
            "EnumProcessModulesEx failed: " + lastErrorToString(::GetLastError()));
    }

    const std::size_t moduleCount = bytesReturned / sizeof(HMODULE);
    std::vector<NativeModuleInfo> result;
    result.reserve(moduleCount);

    for (std::size_t i = 0; i < moduleCount; ++i) {
        HMODULE moduleHandle = moduleHandles[i];

        MODULEINFO moduleInfo{};
        if (::GetModuleInformation(winHandle, moduleHandle, &moduleInfo, sizeof(moduleInfo)) == 0) {
            // Skip modules we can't query (e.g. it unloaded mid-enumeration)
            // rather than failing the entire enumeration.
            continue;
        }

        std::array<char, MAX_PATH> pathBuffer{};
        const DWORD pathLength = ::GetModuleFileNameExA(
            winHandle, moduleHandle, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));

        NativeModuleInfo info;
        info.baseAddress = reinterpret_cast<rmg::core::Address>(moduleInfo.lpBaseOfDll);
        info.size = moduleInfo.SizeOfImage;

        if (pathLength > 0) {
            info.path.assign(pathBuffer.data(), pathLength);
            info.name = extractFileName(info.path);
        }

        result.push_back(std::move(info));
    }

    return result;
}

} // namespace rmg::platform::detail
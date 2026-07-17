// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/windows/win_process_handle.hpp
//
// Internal (non-installed) header shared by every Windows platform backend
// translation unit. Provides:
//   - Conversions between rmg::platform::NativeHandle and the real Win32
//     HANDLE type.
//   - Protection-flag translation between Win32 PAGE_* constants and
//     rmg::core::MemoryProtection.
//   - Free functions implementing the bulk of IPlatformTraits' behavior on
//     Windows, consumed by WindowsPlatformTraits (win_platform_traits.cpp).
//
// This header is intentionally NOT installed alongside the public API: it
// is a private implementation detail of the Windows backend only.
// ==============================================================================

#pragma once

#include <rmg/core/error.hpp>
#include <rmg/core/types.hpp>
#include <rmg/platform/memory_region.hpp>
#include <rmg/platform/native_types.hpp>
#include <rmg/platform/platform_traits.hpp>
#include <rmg/platform/process_handle.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace rmg::platform::detail {

/// @brief Reinterprets an rmg NativeHandle as the real Win32 HANDLE.
///
/// Implemented as a plain reinterpret via void*, since on Windows
/// NativeHandle already *is* HANDLE (both are void*); this indirection
/// exists purely so translation units that need the concept of "the real
/// Win32 type" stay documented and greppable.
void* toWinHandle(NativeHandle handle) noexcept;

/// @brief Wraps a raw Win32 HANDLE as an rmg NativeHandle.
NativeHandle fromWinHandle(void* winHandle) noexcept;

/// @brief Translates a Win32 PAGE_* protection constant (as returned by
///        VirtualQueryEx) into rmg::core::MemoryProtection flags.
[[nodiscard]] rmg::core::MemoryProtection toRmgProtection(std::uint32_t winProtect) noexcept;

/// @brief Formats a Win32 error code (as returned by GetLastError()) into a
///        human-readable string suitable for Error context messages.
[[nodiscard]] std::string lastErrorToString(std::uint32_t errorCode);

/// @brief Implements IPlatformTraits::enumerateRegions on Windows via
///        VirtualQueryEx, resolving backing-file names with
///        GetMappedFileNameA where available.
[[nodiscard]] rmg::core::Result<std::vector<MemoryRegion>>
enumerateRegionsWindows(const ProcessHandle& handle);

/// @brief Implements IPlatformTraits::enumerateModules on Windows via
///        EnumProcessModulesEx + GetModuleInformation + GetModuleFileNameExA.
[[nodiscard]] rmg::core::Result<std::vector<NativeModuleInfo>>
enumerateModulesWindows(const ProcessHandle& handle);

/// @brief Implements IPlatformTraits::readMemory on Windows via
///        ReadProcessMemory.
[[nodiscard]] rmg::core::Result<std::size_t>
readMemoryWindows(const ProcessHandle& handle, rmg::core::Address address,
                  rmg::core::MutableByteView destination);

/// @brief Implements IPlatformTraits::queryProtection on Windows via
///        VirtualQueryEx.
[[nodiscard]] rmg::core::Result<rmg::core::MemoryProtection>
queryProtectionWindows(const ProcessHandle& handle, rmg::core::Address address);

} // namespace rmg::platform::detail
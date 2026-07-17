// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/linux/linux_process_handle.hpp
//
// Internal (non-installed) header shared by every Linux platform backend
// translation unit. On Linux, RMG performs memory reads via pread() on
// /proc/[pid]/mem, and derives region/module information by parsing
// /proc/[pid]/maps. This header declares the shared helpers used by
// linux_platform_traits.cpp and friends.
// ==============================================================================

#pragma once

#include <rmg/core/error.hpp>
#include <rmg/core/types.hpp>
#include <rmg/platform/memory_region.hpp>
#include <rmg/platform/native_types.hpp>
#include <rmg/platform/platform_traits.hpp>
#include <rmg/platform/process_handle.hpp>

#include <string>
#include <vector>

namespace rmg::platform::detail {

/// @brief Translates the rwxp-style permission string found in
///        /proc/[pid]/maps (e.g. "r-xp") into rmg::core::MemoryProtection.
[[nodiscard]] rmg::core::MemoryProtection
parseProtectionString(std::string_view permissions) noexcept;

/// @brief Formats an errno value into a human-readable string suitable for
///        Error context messages, e.g. "[13] Permission denied".
[[nodiscard]] std::string errnoToString(int errnoValue);

/// @brief Parses the full contents of /proc/[pid]/maps into a list of
///        MemoryRegion entries. Exposed separately from
///        enumerateRegionsLinux() so it can be unit-tested against a
///        synthetic maps-file string without requiring a real process.
[[nodiscard]] std::vector<MemoryRegion> parseProcMaps(std::string_view mapsContent);

/// @brief Reads the entire contents of /proc/[pid]/maps for the process
///        referenced by @p handle.
[[nodiscard]] rmg::core::Result<std::string> readProcMapsFile(const ProcessHandle& handle);

/// @brief Implements IPlatformTraits::enumerateRegions on Linux by reading
///        and parsing /proc/[pid]/maps.
[[nodiscard]] rmg::core::Result<std::vector<MemoryRegion>>
enumerateRegionsLinux(const ProcessHandle& handle);

/// @brief Implements IPlatformTraits::enumerateModules on Linux by
///        collapsing contiguous /proc/[pid]/maps entries that share the
///        same backing file into single module descriptors.
[[nodiscard]] rmg::core::Result<std::vector<NativeModuleInfo>>
enumerateModulesLinux(const ProcessHandle& handle);

/// @brief Implements IPlatformTraits::readMemory on Linux via pread() on
///        the process' /proc/[pid]/mem file descriptor owned by @p handle.
[[nodiscard]] rmg::core::Result<std::size_t>
readMemoryLinux(const ProcessHandle& handle, rmg::core::Address address,
                rmg::core::MutableByteView destination);

/// @brief Implements IPlatformTraits::queryProtection on Linux by parsing
///        /proc/[pid]/maps and locating the region containing @p address.
[[nodiscard]] rmg::core::Result<rmg::core::MemoryProtection>
queryProtectionLinux(const ProcessHandle& handle, rmg::core::Address address);

} // namespace rmg::platform::detail
// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/platform/platform_traits.hpp
//
// Strategy-pattern interface that decouples all higher-level RMG components
// (MemoryScanner, ModuleEnumerator, IntegrityChecker, ...) from the concrete
// operating system they run on. Every OS interaction that the rest of the
// library needs is expressed here as a pure virtual method; concrete
// implementations live in src/platform/windows and src/platform/linux.
//
// This is the single Dependency Inversion seam of the whole library: no
// header outside include/rmg/platform/ and src/platform/ should ever
// #include an OS header directly.
// ==============================================================================

#pragma once

#include <cstddef>
#include <vector>

#include <rmg/core/error.hpp>
#include <rmg/core/types.hpp>
#include <rmg/platform/memory_region.hpp>
#include <rmg/platform/process_handle.hpp>

namespace rmg::platform {

/// @brief Platform-neutral description of a loaded module (executable or
///        shared library) within a process, as reported directly by the OS
///        loader. This is intentionally minimal — richer module metadata
///        (code sections, etc.) is built on top of this by
///        rmg::modules::ModuleEnumerator.
struct NativeModuleInfo {
    std::string name;
    std::string path;
    rmg::core::Address baseAddress = 0;
    std::size_t size = 0;
};

/// @brief Abstract interface exposing every OS-specific capability that RMG
///        requires. One concrete implementation exists per supported
///        platform (WindowsPlatformTraits, LinuxPlatformTraits).
class IPlatformTraits {
public:
    virtual ~IPlatformTraits() = default;

    /// @brief Enumerates every mapped memory region belonging to the
    ///        process referenced by @p handle.
    [[nodiscard]] virtual rmg::core::Result<std::vector<MemoryRegion>>
    enumerateRegions(const ProcessHandle& handle) const = 0;

    /// @brief Enumerates every module (executable + shared libraries)
    ///        currently loaded into the process referenced by @p handle.
    [[nodiscard]] virtual rmg::core::Result<std::vector<NativeModuleInfo>>
    enumerateModules(const ProcessHandle& handle) const = 0;

    /// @brief Reads @p size bytes starting at @p address from the process
    ///        referenced by @p handle into @p destination.
    ///
    /// @param destination Must be at least @p size bytes; the actual number
    ///                     of bytes written is returned on success (a
    ///                     partial read is possible near the end of a
    ///                     mapped region and is not itself an error).
    [[nodiscard]] virtual rmg::core::Result<std::size_t>
    readMemory(const ProcessHandle& handle,
               rmg::core::Address address,
               rmg::core::MutableByteView destination) const = 0;

    /// @brief Queries the protection flags currently applied at @p address
    ///        in the process referenced by @p handle.
    [[nodiscard]] virtual rmg::core::Result<rmg::core::MemoryProtection>
    queryProtection(const ProcessHandle& handle, rmg::core::Address address) const = 0;

    /// @brief Returns the OS-reported page size for the current platform,
    ///        used to align region-boundary calculations.
    [[nodiscard]] virtual std::size_t pageSize() const noexcept = 0;
};

} // namespace rmg::platform
// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/platform/memory_region.hpp
//
// Platform-neutral description of a single mapped memory region within a
// process' address space, as reported by the OS (VirtualQueryEx on Windows,
// /proc/[pid]/maps on Linux). This is the fundamental unit that
// MemoryRegionEnumerator produces and that MemoryScanner/IntegrityChecker
// consume.
// ==============================================================================

#pragma once

#include <string>

#include <rmg/core/types.hpp>

namespace rmg::platform {

/// @brief Describes one contiguous, uniformly-protected region of mapped
///        memory within a process.
struct MemoryRegion {
    /// @brief Base virtual address of the region within the owning process.
    rmg::core::Address baseAddress = 0;

    /// @brief Size of the region, in bytes.
    std::size_t size = 0;

    /// @brief Protection flags currently applied to the region.
    rmg::core::MemoryProtection protection = rmg::core::MemoryProtection::None;

    /// @brief Best-effort name of the module that owns this region (e.g.
    ///        "kernel32.dll", "libc.so.6"), or empty if the region is
    ///        anonymous (heap, stack, JIT-allocated memory, etc.).
    std::string moduleName;

    /// @brief Full path to the backing file, if any (empty for anonymous
    ///        mappings).
    std::string modulePath;

    /// @brief True if the region is committed/mapped and readable by the
    ///        OS; regions that are merely reserved (Windows MEM_RESERVE) or
    ///        marked as guard pages are reported with isCommitted == false.
    bool isCommitted = true;

    /// @brief Convenience accessor: true if the region grants execute
    ///        permission, making it a candidate for code-integrity and hook
    ///        scanning.
    [[nodiscard]] bool isExecutable() const noexcept {
        return rmg::core::hasFlag(protection, rmg::core::MemoryProtection::Execute);
    }

    /// @brief Convenience accessor: true if the region is writable, which is
    ///        notable for executable regions (W^X violations are a common
    ///        hook/injection indicator).
    [[nodiscard]] bool isWritable() const noexcept {
        return rmg::core::hasFlag(protection, rmg::core::MemoryProtection::Write);
    }

    /// @brief The exclusive end address of the region (baseAddress + size).
    [[nodiscard]] rmg::core::Address endAddress() const noexcept {
        return baseAddress + static_cast<rmg::core::Address>(size);
    }

    /// @brief True if @p address falls within [baseAddress, endAddress()).
    [[nodiscard]] bool contains(rmg::core::Address address) const noexcept {
        return address >= baseAddress && address < endAddress();
    }
};

} // namespace rmg::platform
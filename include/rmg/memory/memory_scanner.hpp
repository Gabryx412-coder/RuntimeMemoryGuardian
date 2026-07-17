// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/memory/memory_scanner.hpp
//
// The central component for reading and scanning the memory of a process
// (either the current process, for self-integrity checks, or a remote
// target process). Combines region enumeration and content capture into a
// single, convenient API surface consumed by IntegrityChecker and
// HookDetector.
// ==============================================================================

#pragma once

#include <rmg/core/error.hpp>
#include <rmg/core/types.hpp>
#include <rmg/memory/memory_region_enumerator.hpp>
#include <rmg/memory/memory_snapshot.hpp>
#include <rmg/platform/platform_traits.hpp>
#include <rmg/platform/process_handle.hpp>
#include <rmg/utils/byte_buffer.hpp>

#include <optional>
#include <string>
#include <vector>

namespace rmg::memory {

/// @brief Filters applied when selecting which regions a scan should cover.
///
/// All filters are combined with logical AND. Leaving every field at its
/// default produces "all committed regions".
struct ScanFilter {
    /// @brief If set, only regions belonging to the named module are
    ///        included (case-insensitive comparison via rmg::utils::iequals).
    std::optional<std::string> moduleName;

    /// @brief If true, only executable regions are included. Defaults to
    ///        false so a general-purpose scan sees the full address space;
    ///        integrity/hook-oriented callers set this to true explicitly.
    bool executableOnly = false;

    /// @brief If true, excludes regions that are both writable and
    ///        executable — a common indicator of suspicious memory
    ///        (legitimate code sections are normally read-only + execute).
    bool excludeWritableExecutable = false;
};

/// @brief Reads and scans the memory of a target process.
///
/// MemoryScanner owns neither the ProcessHandle nor the IPlatformTraits it
/// is given — both are supplied per-call so a single scanner instance can,
/// in principle, be reused across different targets sharing the same
/// platform backend.
class MemoryScanner final {
public:
    explicit MemoryScanner(const rmg::platform::IPlatformTraits& platformTraits) noexcept
        : platformTraits_(&platformTraits), regionEnumerator_(platformTraits) {}

    /// @brief Captures a MemorySnapshot of every region in @p handle that
    ///        matches @p filter.
    [[nodiscard]] rmg::core::Result<MemorySnapshot> scan(const rmg::platform::ProcessHandle& handle,
                                                         const ScanFilter& filter = {}) const;

    /// @brief Reads exactly @p size bytes starting at @p address from
    ///        @p handle, returning them as an owning ByteBuffer.
    ///
    /// Unlike scan(), this performs a single direct read without going
    /// through region enumeration first — useful when the caller already
    /// knows the exact address range of interest (e.g. HookDetector reading
    /// a function prologue).
    [[nodiscard]] rmg::core::Result<rmg::utils::ByteBuffer>
    read(const rmg::platform::ProcessHandle& handle, rmg::core::Address address,
         std::size_t size) const;

private:
    [[nodiscard]] bool matchesFilter(const rmg::platform::MemoryRegion& region,
                                     const ScanFilter& filter) const;

    const rmg::platform::IPlatformTraits* platformTraits_;
    MemoryRegionEnumerator regionEnumerator_;
};

} // namespace rmg::memory
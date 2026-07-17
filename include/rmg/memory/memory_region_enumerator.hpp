// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/memory/memory_region_enumerator.hpp
//
// Thin domain-layer wrapper around IPlatformTraits::enumerateRegions.
// Exists as a separate component (rather than folding directly into
// MemoryScanner) so that "listing regions" and "reading their contents" are
// independently testable and independently reusable — ModuleEnumerator, for
// instance, builds on region data without needing MemoryScanner's read
// logic at all.
// ==============================================================================

#pragma once

#include <rmg/core/error.hpp>
#include <rmg/platform/memory_region.hpp>
#include <rmg/platform/platform_traits.hpp>
#include <rmg/platform/process_handle.hpp>

#include <vector>

namespace rmg::memory {

/// @brief Enumerates the mapped memory regions of a target process via an
///        injected IPlatformTraits backend.
///
/// Depending on IPlatformTraits (an interface) rather than a concrete
/// platform backend allows this class — and everything built on top of it —
/// to be unit-tested with a mock backend, without spawning real processes.
class MemoryRegionEnumerator final {
public:
    /// @param platformTraits Non-owning reference to the platform backend
    ///                       used to perform the actual enumeration. Must
    ///                       outlive this object.
    explicit MemoryRegionEnumerator(const rmg::platform::IPlatformTraits& platformTraits) noexcept
        : platformTraits_(&platformTraits) {}

    /// @brief Enumerates every mapped region of the process referenced by
    ///        @p handle.
    [[nodiscard]] rmg::core::Result<std::vector<rmg::platform::MemoryRegion>>
    enumerate(const rmg::platform::ProcessHandle& handle) const;

    /// @brief Enumerates only regions that are both committed and
    ///        executable — the subset relevant to code-integrity and hook
    ///        scanning.
    [[nodiscard]] rmg::core::Result<std::vector<rmg::platform::MemoryRegion>>
    enumerateExecutable(const rmg::platform::ProcessHandle& handle) const;

private:
    const rmg::platform::IPlatformTraits* platformTraits_;
};

} // namespace rmg::memory
// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/modules/module_enumerator.hpp
//
// Enumerates the modules currently loaded in a target process and enriches
// the raw platform-level data (rmg::platform::NativeModuleInfo) with
// code-section information suitable for integrity baselining and hook
// scanning. This is the bridge between the OS-facing platform/ layer and
// the domain-facing integrity/ and hooks/ layers.
// ==============================================================================

#pragma once

#include <vector>

#include <rmg/core/error.hpp>
#include <rmg/modules/module_info.hpp>
#include <rmg/platform/platform_traits.hpp>
#include <rmg/platform/process_handle.hpp>

namespace rmg::modules {

/// @brief Enumerates loaded modules and derives their executable code
///        sections via memory-region inspection.
class ModuleEnumerator final {
public:
    explicit ModuleEnumerator(const rmg::platform::IPlatformTraits& platformTraits) noexcept
        : platformTraits_(&platformTraits) {}

    /// @brief Enumerates every module currently loaded in the process
    ///        referenced by @p handle.
    ///
    /// Code sections for each module are derived by intersecting the
    /// module's address range with the process' executable memory regions
    /// (via IPlatformTraits::enumerateRegions), since this works
    /// identically on both platforms without requiring an executable-image
    /// format parser. This yields coarse-grained sections (one per
    /// contiguous executable mapping) rather than named PE/ELF sections;
    /// see docs/architecture/HOOK_DETECTOR.md for the planned refinement
    /// using a dedicated image parser.
    [[nodiscard]] rmg::core::Result<std::vector<ModuleInfo>>
    enumerate(const rmg::platform::ProcessHandle& handle) const;

    /// @brief Enumerates and returns only the single module named
    ///        @p moduleName (case-insensitive match), or a NotFound error if
    ///        no such module is currently loaded.
    [[nodiscard]] rmg::core::Result<ModuleInfo>
    findByName(const rmg::platform::ProcessHandle& handle, std::string_view moduleName) const;

private:
    const rmg::platform::IPlatformTraits* platformTraits_;
};

} // namespace rmg::modules
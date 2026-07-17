// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/integrity/code_section_info.hpp
//
// Models a single executable code section (e.g. a PE ".text" section or an
// ELF PT_LOAD segment with execute permission) that RMG treats as a unit of
// integrity verification. Produced by rmg::modules::ModuleEnumerator and
// consumed by IntegrityBaseline / IntegrityChecker.
// ==============================================================================

#pragma once

#include <rmg/core/types.hpp>

#include <string>

namespace rmg::integrity {

/// @brief Describes one executable code section belonging to a module.
struct CodeSectionInfo {
    /// @brief Section name if known (e.g. ".text"), otherwise a
    ///        synthesized placeholder such as "region_0". Not used for
    ///        identity — @c baseAddress is authoritative.
    std::string name;

    /// @brief Absolute base address of the section within the process.
    rmg::core::Address baseAddress = 0;

    /// @brief Size of the section, in bytes.
    std::size_t size = 0;

    /// @brief Name of the module that owns this section (e.g. "ntdll.dll",
    ///        "libc.so.6"), used for reporting and for grouping sections
    ///        that belong to the same baseline entry.
    std::string ownerModule;

    /// @brief Full path to the module's backing file on disk, when known.
    std::string ownerModulePath;

    [[nodiscard]] rmg::core::Address endAddress() const noexcept {
        return baseAddress + static_cast<rmg::core::Address>(size);
    }

    [[nodiscard]] bool operator==(const CodeSectionInfo&) const = default;
};

} // namespace rmg::integrity
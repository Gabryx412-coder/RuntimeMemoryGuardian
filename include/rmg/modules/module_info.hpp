// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/modules/module_info.hpp
//
// Rich, RMG-level description of a loaded module (executable or shared
// library), combining the raw platform-level NativeModuleInfo with the
// code-section metadata needed by the integrity and hook-detection
// subsystems. Produced by rmg::modules::ModuleEnumerator; consumed by
// IntegrityBaseline, HookDetector, and ModuleMonitor.
// ==============================================================================

#pragma once

#include <string>
#include <vector>

#include <rmg/core/types.hpp>
#include <rmg/integrity/code_section_info.hpp>

namespace rmg::modules {

/// @brief Describes one loaded module and its constituent code sections.
struct ModuleInfo {
    /// @brief File name of the module (e.g. "kernel32.dll", "libc.so.6").
    std::string name;

    /// @brief Full path to the module's backing file on disk.
    std::string path;

    /// @brief Base address at which the module is loaded in the process.
    rmg::core::Address baseAddress = 0;

    /// @brief Total size of the module's in-memory image, in bytes.
    std::size_t size = 0;

    /// @brief Executable code sections belonging to this module, used as
    ///        the unit of integrity/hook scanning.
    std::vector<rmg::integrity::CodeSectionInfo> sections;

    [[nodiscard]] rmg::core::Address endAddress() const noexcept {
        return baseAddress + static_cast<rmg::core::Address>(size);
    }

    [[nodiscard]] bool contains(rmg::core::Address address) const noexcept {
        return address >= baseAddress && address < endAddress();
    }
};

} // namespace rmg::modules
// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/hooks/iat_hook_detector.hpp
//
// Detects Import Address Table (IAT) redirection: a common, well-documented
// technique where a module's table of resolved import addresses is patched
// so that calls intended for a legitimate imported function are silently
// redirected elsewhere. Detection here is purely observational — reading
// the IAT and validating resolved addresses land inside a plausible,
// currently-loaded module — with no modification of the table performed.
// ==============================================================================

#pragma once

#include <vector>

#include <rmg/core/error.hpp>
#include <rmg/hooks/hook_types.hpp>
#include <rmg/memory/memory_scanner.hpp>
#include <rmg/modules/module_info.hpp>
#include <rmg/platform/process_handle.hpp>

namespace rmg::hooks {

/// @brief Detects suspicious Import Address Table entries within a module.
///
/// @note Full IAT parsing requires understanding the target's executable
///       image format (PE import directory on Windows; the ELF PLT/GOT on
///       Linux). This class defines the detection contract and orchestrates
///       the platform-specific table walk, which is implemented behind
///       the ImageFormatReader abstraction (see the module_ variant in the
///       corresponding .cpp) so the detection *logic* here remains
///       platform-neutral.
class IatHookDetector final {
public:
    explicit IatHookDetector(const rmg::memory::MemoryScanner& scanner) noexcept
        : scanner_(&scanner) {}

    /// @brief Scans @p targetModule's import table, flagging entries whose
    ///        resolved address does not fall within any module currently
    ///        listed in @p loadedModules.
    ///
    /// @param loadedModules The full set of modules currently loaded in the
    ///                       target process, used to validate that each IAT
    ///                       entry resolves into a legitimate module image
    ///                       rather than into unmapped, anonymous, or
    ///                       otherwise unexpected memory.
    [[nodiscard]] rmg::core::Result<std::vector<HookFinding>>
    scan(const rmg::platform::ProcessHandle& handle,
         const rmg::modules::ModuleInfo& targetModule,
         std::span<const rmg::modules::ModuleInfo> loadedModules) const;

private:
    const rmg::memory::MemoryScanner* scanner_;
};

} // namespace rmg::hooks
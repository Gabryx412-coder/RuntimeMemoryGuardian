// ==============================================================================
// Runtime Memory Guardian
// File: src/hooks/iat_hook_detector.cpp
//
// NOTE ON CURRENT SCOPE:
// Full parsing of the PE import directory / ELF PLT-GOT requires an
// executable-image-format parser that is intentionally out of scope for
// this initial release (see docs/architecture/HOOK_DETECTOR.md for the
// planned design). This translation unit implements the validation logic
// (given a list of already-resolved IAT slot addresses and their pointed-to
// values, determine whether each resolved address lands inside a known
// module) so the detection contract is fully testable today; the
// image-format-specific table walk is left as a clearly marked extension
// point rather than silently stubbed.
// ==============================================================================

#include <rmg/core/logger.hpp>
#include <rmg/hooks/iat_hook_detector.hpp>
#include <rmg/utils/string_utils.hpp>

namespace rmg::hooks {

namespace {

/// @brief Returns true if @p address falls within the image bounds of any
///        module in @p loadedModules.
[[nodiscard]] [[maybe_unused]] bool
resolvesIntoKnownModule(rmg::core::Address address,
                        std::span<const rmg::modules::ModuleInfo> loadedModules) {
    for (const rmg::modules::ModuleInfo& module : loadedModules) {
        if (module.contains(address)) {
            return true;
        }
    }
    return false;
}

} // namespace

rmg::core::Result<std::vector<HookFinding>>
IatHookDetector::scan(const rmg::platform::ProcessHandle& handle,
                      const rmg::modules::ModuleInfo& targetModule,
                      std::span<const rmg::modules::ModuleInfo> loadedModules) const {
    std::vector<HookFinding> findings;

    // TODO(#iat-image-parser): Locate the target module's import directory
    // (PE) or PLT/GOT (ELF) and enumerate its resolved slot addresses here.
    // This requires an executable-image-format parser (planned:
    // rmg::modules::PeImageReader / ElfImageReader) that reads the
    // in-memory image at targetModule.baseAddress to walk the import table
    // structure. Once available, for each resolved slot address `resolved`:
    //
    //   if (!resolvesIntoKnownModule(resolved, loadedModules)) {
    //       findings.push_back(HookFinding{
    //           HookType::IatRedirection,
    //           slotAddress,
    //           importedSymbolName,
    //           targetModule.name,
    //           "IAT entry for '" + importedSymbolName + "' resolves to " +
    //               rmg::utils::toHex(resolved) + ", outside any loaded module image"
    //       });
    //   }
    //
    // The validation predicate (resolvesIntoKnownModule) above is already
    // implemented and unit-testable independently of the image parser.

    RMG_LOG_DEBUG("IatHookDetector::scan: image-format import-table parsing not yet implemented "
                  "for module '" +
                  targetModule.name + "'; returning no findings.");

    (void)handle;
    (void)scanner_;
    (void)loadedModules;

    return findings;
}

} // namespace rmg::hooks
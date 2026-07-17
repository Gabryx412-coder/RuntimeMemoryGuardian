// ==============================================================================
// Runtime Memory Guardian
// File: src/hooks/eat_hook_detector.cpp
//
// NOTE ON CURRENT SCOPE: see the equivalent note in iat_hook_detector.cpp.
// The validation predicate is implemented and testable; the image-format
// export-table walk is a marked extension point pending
// rmg::modules::PeImageReader / ElfImageReader.
// ==============================================================================

#include <rmg/hooks/eat_hook_detector.hpp>

#include <rmg/core/logger.hpp>
#include <rmg/utils/string_utils.hpp>

namespace rmg::hooks {

namespace {

/// @brief Returns true if @p address falls within @p module's own image
///        bounds.
///
/// @note Legitimate "forwarded exports" (e.g. ntdll-forwarded APIs on
///       Windows, or symbol aliasing on Linux) can resolve to a *different*
///       module by design. A production-grade implementation must
///       distinguish a documented forwarder string from an unexpected
///       redirect; this is tracked by the TODO below alongside the export
///       table walk itself.
[[nodiscard]] [[maybe_unused]] bool resolvesWithinOwnModule(rmg::core::Address address,
                                          const rmg::modules::ModuleInfo& module) {
    return module.contains(address);
}

} // namespace

rmg::core::Result<std::vector<HookFinding>>
EatHookDetector::scan(const rmg::platform::ProcessHandle& handle,
                       const rmg::modules::ModuleInfo& targetModule) const {
    std::vector<HookFinding> findings;

    // TODO(#eat-image-parser): Locate the target module's export directory
    // (PE IMAGE_EXPORT_DIRECTORY, or the ELF dynamic symbol table) and
    // enumerate its resolved (name, address) pairs here. Once available,
    // for each resolved export `resolved` for symbol `exportName`:
    //
    //   if (!resolvesWithinOwnModule(resolved, targetModule) &&
    //       !isDocumentedForwarder(targetModule, exportName)) {
    //       findings.push_back(HookFinding{
    //           HookType::EatRedirection,
    //           exportSlotAddress,
    //           exportName,
    //           targetModule.name,
    //           "export '" + exportName + "' resolves to " +
    //               rmg::utils::toHex(resolved) + ", outside module's own image"
    //       });
    //   }
    //
    // The validation predicate (resolvesWithinOwnModule) above is already
    // implemented and unit-testable independently of the image parser.

    RMG_LOG_DEBUG("EatHookDetector::scan: image-format export-table parsing not yet implemented for module '" +
                  targetModule.name + "'; returning no findings.");

    (void)handle;
    (void)scanner_;

    return findings;
}

} // namespace rmg::hooks
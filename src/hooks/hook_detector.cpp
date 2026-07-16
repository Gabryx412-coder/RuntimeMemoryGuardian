// ==============================================================================
// Runtime Memory Guardian
// File: src/hooks/hook_detector.cpp
// ==============================================================================

#include <rmg/hooks/hook_detector.hpp>

#include <rmg/core/logger.hpp>

namespace rmg::hooks {

rmg::core::Result<std::vector<HookFinding>>
HookDetector::scanAll(const rmg::platform::ProcessHandle& handle,
                       const rmg::modules::ModuleInfo& targetModule,
                       std::span<const rmg::modules::ModuleInfo> loadedModules) const {
    std::vector<HookFinding> aggregated;

    auto inlineFindings = inlineDetector_.scan(
        handle, targetModule.sections, targetModule.baseAddress, targetModule.size);
    if (!inlineFindings) {
        return std::unexpected(inlineFindings.error());
    }
    aggregated.insert(aggregated.end(),
                      std::make_move_iterator(inlineFindings->begin()),
                      std::make_move_iterator(inlineFindings->end()));

    auto iatFindings = iatDetector_.scan(handle, targetModule, loadedModules);
    if (!iatFindings) {
        RMG_LOG_WARNING("HookDetector::scanAll: IAT scan failed for '" + targetModule.name +
                        "': " + iatFindings.error().toDiagnosticString());
    } else {
        aggregated.insert(aggregated.end(),
                          std::make_move_iterator(iatFindings->begin()),
                          std::make_move_iterator(iatFindings->end()));
    }

    auto eatFindings = eatDetector_.scan(handle, targetModule);
    if (!eatFindings) {
        RMG_LOG_WARNING("HookDetector::scanAll: EAT scan failed for '" + targetModule.name +
                        "': " + eatFindings.error().toDiagnosticString());
    } else {
        aggregated.insert(aggregated.end(),
                          std::make_move_iterator(eatFindings->begin()),
                          std::make_move_iterator(eatFindings->end()));
    }

    return aggregated;
}

} // namespace rmg::hooks
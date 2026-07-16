// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/hooks/hook_detector.hpp
//
// Facade that orchestrates InlineHookDetector, IatHookDetector, and
// EatHookDetector behind a single, simple API surface. This is the class
// consumed by the public RuntimeMemoryGuardian facade and by
// ProcessMonitor: callers do not need to know about, or individually
// invoke, each underlying detection strategy.
// ==============================================================================

#pragma once

#include <vector>

#include <rmg/core/error.hpp>
#include <rmg/hooks/eat_hook_detector.hpp>
#include <rmg/hooks/hook_types.hpp>
#include <rmg/hooks/iat_hook_detector.hpp>
#include <rmg/hooks/inline_hook_detector.hpp>
#include <rmg/memory/memory_scanner.hpp>
#include <rmg/modules/module_info.hpp>
#include <rmg/platform/process_handle.hpp>

namespace rmg::hooks {

/// @brief Runs every available hook-detection strategy against a module (or
///        set of modules) and aggregates the results.
class HookDetector final {
public:
    explicit HookDetector(const rmg::memory::MemoryScanner& scanner) noexcept
        : inlineDetector_(scanner), iatDetector_(scanner), eatDetector_(scanner) {}

    /// @brief Runs inline, IAT, and EAT detection against @p targetModule.
    ///
    /// @param loadedModules The full set of currently loaded modules in the
    ///                       target process, required by IatHookDetector to
    ///                       validate resolved import addresses.
    [[nodiscard]] rmg::core::Result<std::vector<HookFinding>>
    scanAll(const rmg::platform::ProcessHandle& handle,
            const rmg::modules::ModuleInfo& targetModule,
            std::span<const rmg::modules::ModuleInfo> loadedModules) const;

private:
    InlineHookDetector inlineDetector_;
    IatHookDetector iatDetector_;
    EatHookDetector eatDetector_;
};

} // namespace rmg::hooks
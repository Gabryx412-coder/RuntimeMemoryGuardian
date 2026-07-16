// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/hooks/eat_hook_detector.hpp
//
// Detects Export Address Table (EAT) redirection: a technique where a
// module's own export table is patched so that a function looked up by
// name resolves to an address outside the module's own image, rather than
// to the module's legitimate implementation. As with IatHookDetector, this
// class defines a fully platform-neutral validation contract; the
// image-format-specific export-table walk is a marked extension point.
// ==============================================================================

#pragma once

#include <vector>

#include <rmg/core/error.hpp>
#include <rmg/hooks/hook_types.hpp>
#include <rmg/memory/memory_scanner.hpp>
#include <rmg/modules/module_info.hpp>
#include <rmg/platform/process_handle.hpp>

namespace rmg::hooks {

/// @brief Detects suspicious Export Address Table entries within a module.
class EatHookDetector final {
public:
    explicit EatHookDetector(const rmg::memory::MemoryScanner& scanner) noexcept
        : scanner_(&scanner) {}

    /// @brief Scans @p targetModule's export table, flagging entries whose
    ///        resolved address falls outside @p targetModule's own image
    ///        (a legitimate export should always resolve within its own
    ///        module, forwarded exports to other modules aside — see the
    ///        implementation note regarding forwarders).
    [[nodiscard]] rmg::core::Result<std::vector<HookFinding>>
    scan(const rmg::platform::ProcessHandle& handle,
         const rmg::modules::ModuleInfo& targetModule) const;

private:
    const rmg::memory::MemoryScanner* scanner_;
};

} // namespace rmg::hooks
// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/hooks/hook_types.hpp
//
// Shared vocabulary types used across every hook-detection strategy
// (inline, IAT, EAT). Kept separate from the detector interfaces so that
// consumers of hook findings (e.g. ProcessMonitor) need not depend on any
// specific detector implementation.
//
// IMPORTANT: All detection here is read-only and pattern/heuristic based,
// intended strictly for defensive monitoring. RMG never modifies, restores,
// or removes detected hooks — it only reports them.
// ==============================================================================

#pragma once

#include <string>

#include <rmg/core/types.hpp>

namespace rmg::hooks {

/// @brief Categorizes the detection technique that produced a HookFinding.
enum class HookType : std::uint8_t {
    /// A function's prologue bytes have been overwritten with an
    /// unexpected jump/call, redirecting execution elsewhere (a classic
    /// "inline" or "trampoline" hook).
    InlinePatch,

    /// An entry in a module's Import Address Table points to an address
    /// outside the expected target module, indicating IAT redirection.
    IatRedirection,

    /// An entry in a module's Export Address Table points to an address
    /// outside the module's own image, indicating EAT redirection.
    EatRedirection,

    /// A hook was detected but its precise mechanism could not be
    /// classified into one of the above categories.
    Unknown,
};

/// @brief Returns a short, stable, human-readable name for @p type.
[[nodiscard]] std::string_view toString(HookType type) noexcept;

/// @brief Describes a single suspected hook found by any detector.
struct HookFinding {
    /// @brief Which detection technique produced this finding.
    HookType type = HookType::Unknown;

    /// @brief The address at which the anomaly was observed (e.g. the
    ///        patched function's entry point, or the IAT/EAT slot address).
    rmg::core::Address location = 0;

    /// @brief Name of the imported/exported symbol involved, when
    ///        applicable (empty for inline-patch findings that are not
    ///        symbol-addressed).
    std::string targetSymbol;

    /// @brief Name of the module the finding pertains to (the module whose
    ///        code or import/export table was found to be altered).
    std::string moduleName;

    /// @brief Human-readable explanation suitable for logs and reports,
    ///        e.g. "prologue redirects via near JMP to 0x7ffabc001000,
    ///        outside module image".
    std::string description;
};

} // namespace rmg::hooks
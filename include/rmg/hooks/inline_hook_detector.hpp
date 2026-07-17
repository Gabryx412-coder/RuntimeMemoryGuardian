// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/hooks/inline_hook_detector.hpp
//
// Detects inline ("trampoline") hooks: modifications to a function's
// prologue bytes that redirect execution to an unexpected location. This is
// implemented purely as a documented, defensive pattern-matching technique
// over already-mapped, readable memory — no code execution, injection, or
// modification is performed.
//
// Detection strategy:
//   1. If a baseline byte sequence for the function's expected prologue is
//      known, a direct byte-for-byte mismatch is the strongest signal
//      (delegated to IntegrityChecker in practice).
//   2. Independently of a baseline, this detector also flags prologues that
//      begin with common redirection opcodes (near/far JMP, PUSH+RET) whose
//      target address falls outside the owning module's image — a strong
//      heuristic indicator of hooking regardless of whether a baseline
//      exists yet.
// ==============================================================================

#pragma once

#include <rmg/core/error.hpp>
#include <rmg/hooks/hook_types.hpp>
#include <rmg/integrity/code_section_info.hpp>
#include <rmg/memory/memory_scanner.hpp>
#include <rmg/platform/process_handle.hpp>

#include <span>
#include <vector>

namespace rmg::hooks {

/// @brief Detects inline hooks within one or more code sections by
///        inspecting function-prologue-like byte patterns.
class InlineHookDetector final {
public:
    explicit InlineHookDetector(const rmg::memory::MemoryScanner& scanner) noexcept
        : scanner_(&scanner) {}

    /// @brief Scans every section in @p sections for prologue bytes that
    ///        redirect execution outside the section's owning module image.
    ///
    /// @param moduleBase Base address of the module owning @p sections,
    ///                    used to determine whether a jump target lands
    ///                    "inside" or "outside" the module (a jump to an
    ///                    address inside the same module is far less
    ///                    suspicious than one leaving it entirely).
    /// @param moduleSize Size of the owning module's image, in bytes.
    [[nodiscard]] rmg::core::Result<std::vector<HookFinding>>
    scan(const rmg::platform::ProcessHandle& handle,
         std::span<const rmg::integrity::CodeSectionInfo> sections, rmg::core::Address moduleBase,
         std::size_t moduleSize) const;

private:
    /// @brief Attempts to decode a redirection target from the first bytes
    ///        of a function prologue, recognizing:
    ///          - E9 xx xx xx xx        (near relative JMP rel32)
    ///          - FF 25 xx xx xx xx     (near absolute JMP via [rip+disp32], x64)
    ///          - 68 xx xx xx xx C3     (PUSH imm32; RET  -- a common trampoline idiom)
    ///
    /// @return The resolved absolute target address, or std::nullopt if no
    ///         recognized redirection pattern was found at the start of
    ///         @p prologue.
    [[nodiscard]] std::optional<rmg::core::Address>
    decodeRedirectTarget(rmg::core::ByteView prologue, rmg::core::Address prologueAddress) const;

    const rmg::memory::MemoryScanner* scanner_;

    /// @brief Number of leading bytes inspected per function for
    ///        redirection patterns. Five bytes covers a full near-relative
    ///        JMP (E9 + rel32); larger patterns are checked opportunistically
    ///        when more bytes are available.
    static constexpr std::size_t PROLOGUE_INSPECTION_LENGTH = 16;
};

} // namespace rmg::hooks
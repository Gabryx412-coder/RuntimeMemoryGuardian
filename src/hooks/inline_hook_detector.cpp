// ==============================================================================
// Runtime Memory Guardian
// File: src/hooks/inline_hook_detector.cpp
// ==============================================================================

#include <rmg/core/logger.hpp>
#include <rmg/hooks/inline_hook_detector.hpp>
#include <rmg/utils/string_utils.hpp>

namespace rmg::hooks {

namespace {

[[nodiscard]] std::uint32_t readLE32(rmg::core::ByteView bytes, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset + 0]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

} // namespace

std::optional<rmg::core::Address>
InlineHookDetector::decodeRedirectTarget(rmg::core::ByteView prologue,
                                         rmg::core::Address prologueAddress) const {
    if (prologue.size() < 5) {
        return std::nullopt;
    }

    const auto opcode0 = static_cast<std::uint8_t>(prologue[0]);

    // Pattern 1: E9 rel32  ->  near relative JMP.
    // Target = address-after-instruction + sign-extended rel32.
    if (opcode0 == 0xE9U) {
        const std::int32_t rel32 = static_cast<std::int32_t>(readLE32(prologue, 1));
        const rmg::core::Address instructionEnd = prologueAddress + 5;
        return static_cast<rmg::core::Address>(static_cast<std::int64_t>(instructionEnd) + rel32);
    }

    // Pattern 2: FF 25 disp32  -> near absolute JMP via [rip+disp32] (x64).
    // We report the pointer address itself as the finding location context;
    // resolving the *ultimate* target would require reading the pointer,
    // which callers can do by dereferencing this address if desired.
    if (prologue.size() >= 6 && opcode0 == 0xFFU &&
        static_cast<std::uint8_t>(prologue[1]) == 0x25U) {
        const std::int32_t disp32 = static_cast<std::int32_t>(readLE32(prologue, 2));
        const rmg::core::Address instructionEnd = prologueAddress + 6;
        return static_cast<rmg::core::Address>(static_cast<std::int64_t>(instructionEnd) + disp32);
    }

    // Pattern 3: 68 imm32 C3  -> PUSH imm32; RET (a common trampoline idiom
    // used to redirect execution to an arbitrary absolute address without a
    // longer instruction encoding).
    if (prologue.size() >= 6 && opcode0 == 0x68U &&
        static_cast<std::uint8_t>(prologue[5]) == 0xC3U) {
        return static_cast<rmg::core::Address>(readLE32(prologue, 1));
    }

    return std::nullopt;
}

rmg::core::Result<std::vector<HookFinding>>
InlineHookDetector::scan(const rmg::platform::ProcessHandle& handle,
                         std::span<const rmg::integrity::CodeSectionInfo> sections,
                         rmg::core::Address moduleBase, std::size_t moduleSize) const {
    std::vector<HookFinding> findings;
    const rmg::core::Address moduleEnd = moduleBase + static_cast<rmg::core::Address>(moduleSize);

    for (const rmg::integrity::CodeSectionInfo& section : sections) {
        const std::size_t inspectLength = std::min(section.size, PROLOGUE_INSPECTION_LENGTH);
        if (inspectLength == 0) {
            continue;
        }

        auto buffer = scanner_->read(handle, section.baseAddress, inspectLength);
        if (!buffer) {
            RMG_LOG_DEBUG("InlineHookDetector::scan: cannot read section '" + section.name +
                          "': " + buffer.error().toDiagnosticString());
            continue;
        }

        auto target = decodeRedirectTarget(buffer->view(), section.baseAddress);
        if (!target.has_value()) {
            continue;
        }

        const bool targetOutsideModule = (*target < moduleBase || *target >= moduleEnd);
        if (!targetOutsideModule) {
            // A jump that stays within the same module's image is common in
            // legitimate compiler-generated thunks and is not, by itself, a
            // meaningful indicator.
            continue;
        }

        HookFinding finding;
        finding.type = HookType::InlinePatch;
        finding.location = section.baseAddress;
        finding.moduleName = section.ownerModule;
        finding.description = "section '" + section.name + "' prologue redirects to " +
                              rmg::utils::toHex(*target) + ", outside owning module image [" +
                              rmg::utils::toHex(moduleBase) + ", " + rmg::utils::toHex(moduleEnd) +
                              ")";

        findings.push_back(std::move(finding));
    }

    return findings;
}

} // namespace rmg::hooks
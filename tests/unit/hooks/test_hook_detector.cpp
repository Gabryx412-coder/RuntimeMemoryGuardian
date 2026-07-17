// ==============================================================================
// Runtime Memory Guardian
// File: tests/unit/hooks/test_hook_detector.cpp
//
// Verifies InlineHookDetector's ability to recognize documented redirection
// opcode patterns, and HookDetector's orchestration/aggregation behavior.
// IAT/EAT detectors are verified only for their "no findings when no image
// parser is available yet" contract, consistent with their documented
// TODO scope (see src/hooks/iat_hook_detector.cpp / eat_hook_detector.cpp).
// ==============================================================================

#include <rmg/hooks/hook_detector.hpp>
#include <rmg/memory/memory_scanner.hpp>

#include <gtest/gtest.h>

namespace {

using rmg::core::MutableByteView;
using rmg::platform::MemoryRegion;
using rmg::platform::ProcessHandle;

/// @brief Fixed-content platform backend representing a single module with
///        one code section, whose bytes are fully controlled by the test.
class FixedModulePlatform final : public rmg::platform::IPlatformTraits {
public:
    std::vector<std::byte> sectionBytes;
    rmg::core::Address sectionBase = 0x400000;

    [[nodiscard]] rmg::core::Result<std::vector<MemoryRegion>>
    enumerateRegions(const ProcessHandle&) const override {
        return std::vector<MemoryRegion>{
            MemoryRegion{sectionBase, sectionBytes.size(),
                         rmg::core::MemoryProtection::Read | rmg::core::MemoryProtection::Execute,
                         "", "", true}};
    }

    [[nodiscard]] rmg::core::Result<std::vector<rmg::platform::NativeModuleInfo>>
    enumerateModules(const ProcessHandle&) const override {
        return std::vector<rmg::platform::NativeModuleInfo>{};
    }

    [[nodiscard]] rmg::core::Result<std::size_t>
    readMemory(const ProcessHandle&, rmg::core::Address address,
               MutableByteView destination) const override {
        if (address < sectionBase || address >= sectionBase + sectionBytes.size()) {
            return rmg::core::fail<std::size_t>(rmg::core::ErrorCode::RegionNotFound,
                                                "out of range");
        }
        const std::size_t offset = static_cast<std::size_t>(address - sectionBase);
        const std::size_t available = sectionBytes.size() - offset;
        const std::size_t toCopy = std::min(available, destination.size());
        std::copy_n(sectionBytes.begin() + static_cast<std::ptrdiff_t>(offset), toCopy,
                    destination.begin());
        return toCopy;
    }

    [[nodiscard]] rmg::core::Result<rmg::core::MemoryProtection>
    queryProtection(const ProcessHandle&, rmg::core::Address) const override {
        return rmg::core::MemoryProtection::Read | rmg::core::MemoryProtection::Execute;
    }

    [[nodiscard]] std::size_t pageSize() const noexcept override { return 4096; }
};

TEST(InlineHookDetectorTest, CleanPrologueProducesNoFinding) {
    FixedModulePlatform platform;
    // A benign prologue: push rbp; mov rbp, rsp; ... (no redirection opcode).
    platform.sectionBytes = {std::byte{0x55}, std::byte{0x48}, std::byte{0x89}, std::byte{0xE5},
                             std::byte{0x90}, std::byte{0x90}, std::byte{0x90}, std::byte{0x90},
                             std::byte{0x90}, std::byte{0x90}, std::byte{0x90}, std::byte{0x90},
                             std::byte{0x90}, std::byte{0x90}, std::byte{0x90}, std::byte{0x90}};

    rmg::memory::MemoryScanner scanner(platform);
    rmg::hooks::InlineHookDetector detector(scanner);

    rmg::integrity::CodeSectionInfo section;
    section.name = ".text";
    section.baseAddress = platform.sectionBase;
    section.size = platform.sectionBytes.size();
    section.ownerModule = "clean.dll";

    auto handle = ProcessHandle::openSelf();
    auto findings = detector.scan(*handle, std::vector{section}, platform.sectionBase, 0x10000);

    ASSERT_TRUE(findings.has_value());
    EXPECT_TRUE(findings->empty());
}

TEST(InlineHookDetectorTest, NearJmpOutsideModuleIsDetected) {
    FixedModulePlatform platform;
    const rmg::core::Address moduleBase = 0x400000;
    const std::size_t moduleSize = 0x10000;
    // A target far outside [moduleBase, moduleBase + moduleSize).
    const std::int32_t rel32 = 0x00500000; // Lands well beyond the module.

    std::vector<std::byte> prologue(16, std::byte{0x90});
    prologue[0] = std::byte{0xE9}; // near JMP rel32
    prologue[1] = static_cast<std::byte>(rel32 & 0xFF);
    prologue[2] = static_cast<std::byte>((rel32 >> 8) & 0xFF);
    prologue[3] = static_cast<std::byte>((rel32 >> 16) & 0xFF);
    prologue[4] = static_cast<std::byte>((rel32 >> 24) & 0xFF);

    platform.sectionBase = moduleBase;
    platform.sectionBytes = prologue;

    rmg::memory::MemoryScanner scanner(platform);
    rmg::hooks::InlineHookDetector detector(scanner);

    rmg::integrity::CodeSectionInfo section;
    section.name = ".text";
    section.baseAddress = moduleBase;
    section.size = prologue.size();
    section.ownerModule = "hooked.dll";

    auto handle = ProcessHandle::openSelf();
    auto findings = detector.scan(*handle, std::vector{section}, moduleBase, moduleSize);

    ASSERT_TRUE(findings.has_value());
    ASSERT_EQ(findings->size(), 1U);
    EXPECT_EQ((*findings)[0].type, rmg::hooks::HookType::InlinePatch);
    EXPECT_EQ((*findings)[0].location, moduleBase);
}

TEST(InlineHookDetectorTest, JmpTargetInsideModuleIsNotFlagged) {
    FixedModulePlatform platform;
    const rmg::core::Address moduleBase = 0x400000;
    const std::size_t moduleSize = 0x10000;
    // rel32 chosen so the target lands well within [moduleBase, moduleBase+moduleSize).
    const std::int32_t rel32 = 0x100;

    std::vector<std::byte> prologue(16, std::byte{0x90});
    prologue[0] = std::byte{0xE9};
    prologue[1] = static_cast<std::byte>(rel32 & 0xFF);
    prologue[2] = static_cast<std::byte>((rel32 >> 8) & 0xFF);
    prologue[3] = static_cast<std::byte>((rel32 >> 16) & 0xFF);
    prologue[4] = static_cast<std::byte>((rel32 >> 24) & 0xFF);

    platform.sectionBase = moduleBase;
    platform.sectionBytes = prologue;

    rmg::memory::MemoryScanner scanner(platform);
    rmg::hooks::InlineHookDetector detector(scanner);

    rmg::integrity::CodeSectionInfo section;
    section.name = ".text";
    section.baseAddress = moduleBase;
    section.size = prologue.size();
    section.ownerModule = "normal.dll";

    auto handle = ProcessHandle::openSelf();
    auto findings = detector.scan(*handle, std::vector{section}, moduleBase, moduleSize);

    ASSERT_TRUE(findings.has_value());
    EXPECT_TRUE(findings->empty());
}

TEST(HookTypeTest, ToStringReturnsStableNames) {
    EXPECT_EQ(rmg::hooks::toString(rmg::hooks::HookType::InlinePatch), "InlinePatch");
    EXPECT_EQ(rmg::hooks::toString(rmg::hooks::HookType::IatRedirection), "IatRedirection");
    EXPECT_EQ(rmg::hooks::toString(rmg::hooks::HookType::EatRedirection), "EatRedirection");
    EXPECT_EQ(rmg::hooks::toString(rmg::hooks::HookType::Unknown), "Unknown");
}

TEST(HookDetectorTest, ScanAllAggregatesInlineFindingsAndToleratesEmptyIatEat) {
    FixedModulePlatform platform;
    const rmg::core::Address moduleBase = 0x400000;

    std::vector<std::byte> prologue(16, std::byte{0x90});
    prologue[0] = std::byte{0xE9};
    const std::int32_t rel32 = 0x00700000;
    prologue[1] = static_cast<std::byte>(rel32 & 0xFF);
    prologue[2] = static_cast<std::byte>((rel32 >> 8) & 0xFF);
    prologue[3] = static_cast<std::byte>((rel32 >> 16) & 0xFF);
    prologue[4] = static_cast<std::byte>((rel32 >> 24) & 0xFF);

    platform.sectionBase = moduleBase;
    platform.sectionBytes = prologue;

    rmg::memory::MemoryScanner scanner(platform);
    rmg::hooks::HookDetector detector(scanner);

    rmg::modules::ModuleInfo module;
    module.name = "hooked.dll";
    module.baseAddress = moduleBase;
    module.size = 0x10000;

    rmg::integrity::CodeSectionInfo section;
    section.name = ".text";
    section.baseAddress = moduleBase;
    section.size = prologue.size();
    section.ownerModule = module.name;
    module.sections.push_back(section);

    auto handle = ProcessHandle::openSelf();
    auto findings = detector.scanAll(*handle, module, std::vector{module});

    ASSERT_TRUE(findings.has_value());
    ASSERT_EQ(findings->size(), 1U);
    EXPECT_EQ((*findings)[0].type, rmg::hooks::HookType::InlinePatch);
}

} // namespace
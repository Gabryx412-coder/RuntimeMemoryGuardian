// ==============================================================================
// Runtime Memory Guardian
// File: tests/unit/integrity/test_integrity_checker.cpp
//
// Verifies IntegrityChecker's core guarantee: an unmodified baseline
// verifies as valid, and any modification to a baselined section is
// reported as a TamperedSection.
// ==============================================================================

#include <rmg/integrity/integrity_checker.hpp>
#include <rmg/memory/memory_scanner.hpp>

#include <gtest/gtest.h>

namespace {

using rmg::core::MutableByteView;
using rmg::platform::MemoryRegion;
using rmg::platform::ProcessHandle;

/// @brief Minimal, mutable in-memory platform backend: readMemory() always
///        reflects the current contents of `buffer`, so a test can baseline
///        against one version of the buffer and later mutate it before
///        re-verifying.
class MutableMemoryPlatform final : public rmg::platform::IPlatformTraits {
public:
    std::vector<std::byte> buffer;
    rmg::core::Address baseAddress = 0x2000;

    [[nodiscard]] rmg::core::Result<std::vector<MemoryRegion>>
    enumerateRegions(const ProcessHandle&) const override {
        return std::vector<MemoryRegion>{
            MemoryRegion{baseAddress, buffer.size(),
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
        if (address < baseAddress || address >= baseAddress + buffer.size()) {
            return rmg::core::fail<std::size_t>(rmg::core::ErrorCode::RegionNotFound,
                                                "out of range");
        }
        const std::size_t offset = static_cast<std::size_t>(address - baseAddress);
        const std::size_t available = buffer.size() - offset;
        const std::size_t toCopy = std::min(available, destination.size());
        std::copy_n(buffer.begin() + static_cast<std::ptrdiff_t>(offset), toCopy,
                    destination.begin());
        return toCopy;
    }

    [[nodiscard]] rmg::core::Result<rmg::core::MemoryProtection>
    queryProtection(const ProcessHandle&, rmg::core::Address) const override {
        return rmg::core::MemoryProtection::Read | rmg::core::MemoryProtection::Execute;
    }

    [[nodiscard]] std::size_t pageSize() const noexcept override { return 4096; }
};

class IntegrityCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {
        platform_.buffer = {std::byte{0x90}, std::byte{0x90}, std::byte{0xC3}, std::byte{0x00}};
        handle_ = std::move(*ProcessHandle::openSelf());

        section_.name = ".text";
        section_.baseAddress = platform_.baseAddress;
        section_.size = platform_.buffer.size();
        section_.ownerModule = "test_module";
    }

    MutableMemoryPlatform platform_;
    ProcessHandle handle_ = *ProcessHandle::openSelf();
    rmg::integrity::CodeSectionInfo section_;
};

TEST_F(IntegrityCheckerTest, UnmodifiedSectionVerifiesAsValid) {
    rmg::memory::MemoryScanner scanner(platform_);
    rmg::integrity::Sha256HashProvider hashProvider;

    auto baseline = rmg::integrity::IntegrityBaseline::create(std::vector{section_}, handle_,
                                                              scanner, hashProvider);
    ASSERT_TRUE(baseline.has_value());

    rmg::integrity::IntegrityChecker checker(scanner, hashProvider);
    auto report = checker.verify(handle_, *baseline);

    ASSERT_TRUE(report.has_value());
    EXPECT_TRUE(report->isValid);
    EXPECT_TRUE(report->tamperedSections.empty());
}

TEST_F(IntegrityCheckerTest, ModifiedSectionIsReportedAsTampered) {
    rmg::memory::MemoryScanner scanner(platform_);
    rmg::integrity::Sha256HashProvider hashProvider;

    auto baseline = rmg::integrity::IntegrityBaseline::create(std::vector{section_}, handle_,
                                                              scanner, hashProvider);
    ASSERT_TRUE(baseline.has_value());

    // Simulate an inline patch: overwrite the first byte.
    platform_.buffer[0] = std::byte{0xE9};

    rmg::integrity::IntegrityChecker checker(scanner, hashProvider);
    auto report = checker.verify(handle_, *baseline);

    ASSERT_TRUE(report.has_value());
    EXPECT_FALSE(report->isValid);
    ASSERT_EQ(report->tamperedSections.size(), 1U);
    EXPECT_EQ(report->tamperedSections[0].section.name, ".text");
    EXPECT_NE(report->tamperedSections[0].expectedDigest, report->tamperedSections[0].actualDigest);
}

TEST_F(IntegrityCheckerTest, MismatchedAlgorithmBetweenBaselineAndCheckerFails) {
    rmg::memory::MemoryScanner scanner(platform_);
    rmg::integrity::Sha256HashProvider sha256Provider;
    rmg::integrity::Crc32HashProvider crc32Provider;

    auto baseline = rmg::integrity::IntegrityBaseline::create(std::vector{section_}, handle_,
                                                              scanner, sha256Provider);
    ASSERT_TRUE(baseline.has_value());

    rmg::integrity::IntegrityChecker checker(scanner, crc32Provider);
    auto report = checker.verify(handle_, *baseline);

    ASSERT_FALSE(report.has_value());
    EXPECT_EQ(report.error().code(), rmg::core::ErrorCode::InvalidArgument);
}

TEST_F(IntegrityCheckerTest, BaselineSerializationRoundTripsCorrectly) {
    rmg::memory::MemoryScanner scanner(platform_);
    rmg::integrity::Sha256HashProvider hashProvider;

    auto baseline = rmg::integrity::IntegrityBaseline::create(std::vector{section_}, handle_,
                                                              scanner, hashProvider);
    ASSERT_TRUE(baseline.has_value());

    std::vector<std::byte> serialized = baseline->serialize();
    auto deserialized = rmg::integrity::IntegrityBaseline::deserialize(
        rmg::core::ByteView(serialized.data(), serialized.size()));

    ASSERT_TRUE(deserialized.has_value());
    EXPECT_EQ(deserialized->hashAlgorithmName(), baseline->hashAlgorithmName());
    ASSERT_EQ(deserialized->entries().size(), baseline->entries().size());
    EXPECT_EQ(deserialized->entries()[0].digest, baseline->entries()[0].digest);
    EXPECT_EQ(deserialized->entries()[0].section.name, baseline->entries()[0].section.name);
}

TEST(IntegrityBaselineTest, DeserializeRejectsInvalidMagic) {
    std::vector<std::byte> garbage = {std::byte{'X'}, std::byte{'X'}, std::byte{'X'},
                                      std::byte{'X'}};
    auto result = rmg::integrity::IntegrityBaseline::deserialize(
        rmg::core::ByteView(garbage.data(), garbage.size()));

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), rmg::core::ErrorCode::SerializationError);
}

TEST(IntegrityBaselineTest, DeserializeRejectsTruncatedData) {
    std::vector<std::byte> truncated = {std::byte{'R'}, std::byte{'M'}, std::byte{'G'},
                                        std::byte{'B'}};
    auto result = rmg::integrity::IntegrityBaseline::deserialize(
        rmg::core::ByteView(truncated.data(), truncated.size()));

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), rmg::core::ErrorCode::SerializationError);
}

} // namespace
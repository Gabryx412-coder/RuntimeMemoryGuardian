// ==============================================================================
// Runtime Memory Guardian
// File: tests/unit/memory/test_memory_diff.cpp
// ==============================================================================

#include <rmg/memory/memory_diff.hpp>

#include <gtest/gtest.h>

namespace {

using rmg::platform::MemoryRegion;

/// @brief Constructs a MemorySnapshot-like structure directly for testing
///        MemoryDiff in isolation, bypassing MemorySnapshot::capture()
///        (which requires a real platform backend / process handle).
///
/// Because MemorySnapshot's constructor is private (only accessible via the
/// static capture() factory), this helper uses capture() with a trivial
/// mock backend, keeping the test focused on MemoryDiff's comparison logic
/// while still exercising the real MemorySnapshot type end-to-end.
class DiffTestPlatform final : public rmg::platform::IPlatformTraits {
public:
    std::vector<std::byte> bytes;
    rmg::core::Address baseAddress = 0x1000;

    [[nodiscard]] rmg::core::Result<std::vector<MemoryRegion>>
    enumerateRegions(const rmg::platform::ProcessHandle&) const override {
        return std::vector<MemoryRegion>{MemoryRegion{
            baseAddress, bytes.size(), rmg::core::MemoryProtection::Read, "", "", true}};
    }

    [[nodiscard]] rmg::core::Result<std::vector<rmg::platform::NativeModuleInfo>>
    enumerateModules(const rmg::platform::ProcessHandle&) const override {
        return std::vector<rmg::platform::NativeModuleInfo>{};
    }

    [[nodiscard]] rmg::core::Result<std::size_t>
    readMemory(const rmg::platform::ProcessHandle&, rmg::core::Address address,
               rmg::core::MutableByteView destination) const override {
        if (address != baseAddress) {
            return rmg::core::fail<std::size_t>(rmg::core::ErrorCode::RegionNotFound,
                                                "unexpected address");
        }
        const std::size_t toCopy = std::min(bytes.size(), destination.size());
        std::copy_n(bytes.begin(), toCopy, destination.begin());
        return toCopy;
    }

    [[nodiscard]] rmg::core::Result<rmg::core::MemoryProtection>
    queryProtection(const rmg::platform::ProcessHandle&, rmg::core::Address) const override {
        return rmg::core::MemoryProtection::Read;
    }

    [[nodiscard]] std::size_t pageSize() const noexcept override { return 4096; }
};

[[nodiscard]] rmg::memory::MemorySnapshot captureSnapshot(DiffTestPlatform& platform,
                                                          const std::vector<std::byte>& contents) {
    platform.bytes = contents;
    auto handle = rmg::platform::ProcessHandle::openSelf();
    auto regions = platform.enumerateRegions(*handle);
    auto snapshot = rmg::memory::MemorySnapshot::capture(*handle, *regions, platform);
    return std::move(*snapshot);
}

TEST(MemoryDiffTest, IdenticalSnapshotsProduceNoChanges) {
    DiffTestPlatform platform;
    const std::vector<std::byte> contents = {std::byte{1}, std::byte{2}, std::byte{3},
                                             std::byte{4}};

    auto baseline = captureSnapshot(platform, contents);
    auto current = captureSnapshot(platform, contents);

    auto diff = rmg::memory::MemoryDiff::compare(baseline, current);
    EXPECT_FALSE(diff.hasChanges());
    EXPECT_TRUE(diff.regionSetIdentical);
    EXPECT_TRUE(diff.changedRanges.empty());
}

TEST(MemoryDiffTest, SingleByteChangeIsDetectedWithCorrectAddress) {
    DiffTestPlatform platform;
    const std::vector<std::byte> original = {std::byte{1}, std::byte{2}, std::byte{3},
                                             std::byte{4}};
    const std::vector<std::byte> modified = {std::byte{1}, std::byte{0xFF}, std::byte{3},
                                             std::byte{4}};

    auto baseline = captureSnapshot(platform, original);
    auto current = captureSnapshot(platform, modified);

    auto diff = rmg::memory::MemoryDiff::compare(baseline, current);
    ASSERT_TRUE(diff.hasChanges());
    ASSERT_EQ(diff.changedRanges.size(), 1U);
    EXPECT_EQ(diff.changedRanges[0].address, platform.baseAddress + 1);
    EXPECT_EQ(diff.changedRanges[0].length, 1U);
}

TEST(MemoryDiffTest, ContiguousChangedBytesAreMergedIntoOneRange) {
    DiffTestPlatform platform;
    const std::vector<std::byte> original = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
                                             std::byte{5}};
    const std::vector<std::byte> modified = {std::byte{1}, std::byte{0xA}, std::byte{0xB},
                                             std::byte{0xC}, std::byte{5}};

    auto baseline = captureSnapshot(platform, original);
    auto current = captureSnapshot(platform, modified);

    auto diff = rmg::memory::MemoryDiff::compare(baseline, current);
    ASSERT_EQ(diff.changedRanges.size(), 1U);
    EXPECT_EQ(diff.changedRanges[0].address, platform.baseAddress + 1);
    EXPECT_EQ(diff.changedRanges[0].length, 3U);
}

TEST(MemoryDiffTest, NonContiguousChangesProduceMultipleRanges) {
    DiffTestPlatform platform;
    const std::vector<std::byte> original = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
                                             std::byte{5}};
    const std::vector<std::byte> modified = {std::byte{0xA}, std::byte{2}, std::byte{3},
                                             std::byte{4}, std::byte{0xB}};

    auto baseline = captureSnapshot(platform, original);
    auto current = captureSnapshot(platform, modified);

    auto diff = rmg::memory::MemoryDiff::compare(baseline, current);
    ASSERT_EQ(diff.changedRanges.size(), 2U);
    EXPECT_EQ(diff.changedRanges[0].address, platform.baseAddress);
    EXPECT_EQ(diff.changedRanges[1].address, platform.baseAddress + 4);
}

} // namespace
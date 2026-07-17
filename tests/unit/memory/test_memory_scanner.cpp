// ==============================================================================
// Runtime Memory Guardian
// File: tests/unit/memory/test_memory_scanner.cpp
//
// Exercises MemoryScanner/MemoryRegionEnumerator against a mock
// IPlatformTraits, so these tests run deterministically without depending
// on real OS memory layout.
// ==============================================================================

#include <rmg/memory/memory_region_enumerator.hpp>
#include <rmg/memory/memory_scanner.hpp>

#include <gtest/gtest.h>

namespace {

using rmg::core::ByteView;
using rmg::core::MemoryProtection;
using rmg::core::MutableByteView;
using rmg::platform::IPlatformTraits;
using rmg::platform::MemoryRegion;
using rmg::platform::NativeModuleInfo;
using rmg::platform::ProcessHandle;

/// @brief Test double implementing IPlatformTraits entirely in-memory, with
///        a fixed, caller-configured set of regions and backing bytes. Lets
///        MemoryScanner/MemoryRegionEnumerator be tested without a real
///        target process.
class MockPlatformTraits final : public IPlatformTraits {
public:
    struct FakeRegion {
        MemoryRegion region;
        std::vector<std::byte> contents;
    };

    std::vector<FakeRegion> regions;

    [[nodiscard]] rmg::core::Result<std::vector<MemoryRegion>>
    enumerateRegions(const ProcessHandle&) const override {
        std::vector<MemoryRegion> result;
        result.reserve(regions.size());
        for (const FakeRegion& fake : regions) {
            result.push_back(fake.region);
        }
        return result;
    }

    [[nodiscard]] rmg::core::Result<std::vector<NativeModuleInfo>>
    enumerateModules(const ProcessHandle&) const override {
        return std::vector<NativeModuleInfo>{};
    }

    [[nodiscard]] rmg::core::Result<std::size_t>
    readMemory(const ProcessHandle&, rmg::core::Address address,
               MutableByteView destination) const override {
        for (const FakeRegion& fake : regions) {
            if (!fake.region.contains(address)) {
                continue;
            }
            const std::size_t offset = static_cast<std::size_t>(address - fake.region.baseAddress);
            const std::size_t available = fake.contents.size() - offset;
            const std::size_t toCopy = std::min(available, destination.size());
            std::copy_n(fake.contents.begin() + static_cast<std::ptrdiff_t>(offset), toCopy,
                        destination.begin());
            return toCopy;
        }
        return rmg::core::fail<std::size_t>(rmg::core::ErrorCode::RegionNotFound,
                                            "address not mapped");
    }

    [[nodiscard]] rmg::core::Result<MemoryProtection>
    queryProtection(const ProcessHandle&, rmg::core::Address address) const override {
        for (const FakeRegion& fake : regions) {
            if (fake.region.contains(address)) {
                return fake.region.protection;
            }
        }
        return rmg::core::fail<MemoryProtection>(rmg::core::ErrorCode::RegionNotFound,
                                                 "address not mapped");
    }

    [[nodiscard]] std::size_t pageSize() const noexcept override { return 4096; }
};

/// @brief Constructs a dummy ProcessHandle for use with MockPlatformTraits.
///        MockPlatformTraits never actually dereferences the native handle,
///        so openSelf() (always succeeds, cheap) is a safe, portable choice
///        for obtaining a valid handle object in tests.
[[nodiscard]] ProcessHandle makeDummyHandle() {
    auto handle = ProcessHandle::openSelf();
    return std::move(*handle);
}

TEST(MemoryRegionEnumeratorTest, EnumerateReturnsAllConfiguredRegions) {
    MockPlatformTraits mock;
    mock.regions.push_back({MemoryRegion{0x1000, 0x100, MemoryProtection::Read, "", "", true}, {}});
    mock.regions.push_back({MemoryRegion{0x2000, 0x200, MemoryProtection::Read, "", "", true}, {}});

    rmg::memory::MemoryRegionEnumerator enumerator(mock);
    auto handle = makeDummyHandle();

    auto result = enumerator.enumerate(handle);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2U);
}

TEST(MemoryRegionEnumeratorTest, EnumerateExecutableFiltersNonExecutableRegions) {
    MockPlatformTraits mock;
    mock.regions.push_back({MemoryRegion{0x1000, 0x100, MemoryProtection::Read, "", "", true}, {}});
    mock.regions.push_back(
        {MemoryRegion{0x2000, 0x100, MemoryProtection::Read | MemoryProtection::Execute, "", "",
                      true},
         {}});

    rmg::memory::MemoryRegionEnumerator enumerator(mock);
    auto handle = makeDummyHandle();

    auto result = enumerator.enumerateExecutable(handle);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1U);
    EXPECT_EQ((*result)[0].baseAddress, 0x2000U);
}

TEST(MemoryScannerTest, ReadReturnsExactBytesWhenFullyMapped) {
    MockPlatformTraits mock;
    std::vector<std::byte> data = {std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE},
                                   std::byte{0xEF}};
    mock.regions.push_back(
        {MemoryRegion{0x1000, data.size(), MemoryProtection::Read, "", "", true}, data});

    rmg::memory::MemoryScanner scanner(mock);
    auto handle = makeDummyHandle();

    auto result = scanner.read(handle, 0x1000, data.size());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), data.size());
    EXPECT_EQ((*result)[0], std::byte{0xDE});
    EXPECT_EQ((*result)[3], std::byte{0xEF});
}

TEST(MemoryScannerTest, ReadFromUnmappedAddressFails) {
    MockPlatformTraits mock;
    rmg::memory::MemoryScanner scanner(mock);
    auto handle = makeDummyHandle();

    auto result = scanner.read(handle, 0xDEADBEEF, 16);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), rmg::core::ErrorCode::RegionNotFound);
}

TEST(MemoryScannerTest, ScanWithExecutableOnlyFilterCapturesOnlyExecutableRegions) {
    MockPlatformTraits mock;
    std::vector<std::byte> executableBytes = {std::byte{0x90}, std::byte{0x90}};
    std::vector<std::byte> dataBytes = {std::byte{0x00}, std::byte{0x00}};

    mock.regions.push_back(
        {MemoryRegion{0x1000, 2, MemoryProtection::Read | MemoryProtection::Execute, "", "", true},
         executableBytes});
    mock.regions.push_back(
        {MemoryRegion{0x2000, 2, MemoryProtection::Read | MemoryProtection::Write, "", "", true},
         dataBytes});

    rmg::memory::MemoryScanner scanner(mock);
    auto handle = makeDummyHandle();

    rmg::memory::ScanFilter filter;
    filter.executableOnly = true;

    auto snapshotResult = scanner.scan(handle, filter);
    ASSERT_TRUE(snapshotResult.has_value());
    ASSERT_EQ(snapshotResult->regions().size(), 1U);
    EXPECT_EQ(snapshotResult->regions()[0].region.baseAddress, 0x1000U);
}

TEST(MemoryScannerTest, ScanWithModuleNameFilterMatchesCaseInsensitively) {
    MockPlatformTraits mock;
    MemoryRegion region{0x1000, 4, MemoryProtection::Read, "", "", true};
    region.moduleName = "KERNEL32.DLL";
    mock.regions.push_back({region, {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}}});

    rmg::memory::MemoryScanner scanner(mock);
    auto handle = makeDummyHandle();

    rmg::memory::ScanFilter filter;
    filter.moduleName = "kernel32.dll";

    auto snapshotResult = scanner.scan(handle, filter);
    ASSERT_TRUE(snapshotResult.has_value());
    EXPECT_EQ(snapshotResult->regions().size(), 1U);
}

} // namespace
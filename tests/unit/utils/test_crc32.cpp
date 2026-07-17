// ==============================================================================
// Runtime Memory Guardian
// File: tests/unit/utils/test_crc32.cpp
// ==============================================================================

#include <rmg/utils/crc32.hpp>

#include <gtest/gtest.h>

namespace {

using rmg::core::ByteView;

[[nodiscard]] ByteView asBytes(const std::string& text) {
    return ByteView(reinterpret_cast<const std::byte*>(text.data()), text.size());
}

TEST(Crc32Test, EmptyInputProducesZero) {
    EXPECT_EQ(rmg::utils::crc32(ByteView{}), 0x00000000U);
}

TEST(Crc32Test, KnownVector123456789) {
    // Standard CRC-32 (IEEE 802.3) check value for the ASCII string
    // "123456789" is the well-known 0xCBF43926.
    const auto result = rmg::utils::crc32(asBytes("123456789"));
    EXPECT_EQ(result, 0xCBF43926U);
}

TEST(Crc32Test, DifferentInputsProduceDifferentChecksums) {
    const auto crcA = rmg::utils::crc32(asBytes("hello"));
    const auto crcB = rmg::utils::crc32(asBytes("world"));
    EXPECT_NE(crcA, crcB);
}

TEST(Crc32Test, IdenticalInputsProduceIdenticalChecksums) {
    const auto crcA = rmg::utils::crc32(asBytes("repeatable input"));
    const auto crcB = rmg::utils::crc32(asBytes("repeatable input"));
    EXPECT_EQ(crcA, crcB);
}

TEST(Crc32Test, SingleBitChangeAltersChecksum) {
    std::string original = "The quick brown fox";
    std::string modified = original;
    modified[0] = 'U'; // Flip a bit within the first character.

    EXPECT_NE(rmg::utils::crc32(asBytes(original)), rmg::utils::crc32(asBytes(modified)));
}

} // namespace
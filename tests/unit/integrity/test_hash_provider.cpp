// ==============================================================================
// Runtime Memory Guardian
// File: tests/unit/integrity/test_hash_provider.cpp
// ==============================================================================

#include <gtest/gtest.h>

#include <rmg/integrity/hash_provider.hpp>

namespace {

using rmg::core::ByteView;

[[nodiscard]] ByteView asBytes(const std::string& text) {
    return ByteView(reinterpret_cast<const std::byte*>(text.data()), text.size());
}

TEST(Sha256HashProviderTest, AlgorithmNameIsSha256) {
    rmg::integrity::Sha256HashProvider provider;
    EXPECT_EQ(provider.algorithmName(), "SHA-256");
}

TEST(Sha256HashProviderTest, ProducesThirtyTwoByteDigest) {
    rmg::integrity::Sha256HashProvider provider;
    auto digest = provider.hash(asBytes("test data"));
    EXPECT_EQ(digest.size(), 32U);
}

TEST(Sha256HashProviderTest, IdenticalInputsProduceIdenticalDigests) {
    rmg::integrity::Sha256HashProvider provider;
    auto digestA = provider.hash(asBytes("consistent input"));
    auto digestB = provider.hash(asBytes("consistent input"));
    EXPECT_EQ(digestA, digestB);
}

TEST(Crc32HashProviderTest, AlgorithmNameIsCrc32) {
    rmg::integrity::Crc32HashProvider provider;
    EXPECT_EQ(provider.algorithmName(), "CRC32");
}

TEST(Crc32HashProviderTest, ProducesFourByteDigest) {
    rmg::integrity::Crc32HashProvider provider;
    auto digest = provider.hash(asBytes("test data"));
    EXPECT_EQ(digest.size(), 4U);
}

TEST(MakeDefaultHashProviderTest, ReturnsSha256ByDefault) {
    auto provider = rmg::integrity::makeDefaultHashProvider();
    ASSERT_NE(provider, nullptr);
    EXPECT_EQ(provider->algorithmName(), "SHA-256");
}

TEST(HashProviderTest, DifferentAlgorithmsProduceDifferentDigestsForSameInput) {
    rmg::integrity::Sha256HashProvider sha256;
    rmg::integrity::Crc32HashProvider crc32;

    auto sha256Digest = sha256.hash(asBytes("same input"));
    auto crc32Digest = crc32.hash(asBytes("same input"));

    // Different lengths alone prove they're distinct digest schemes.
    EXPECT_NE(sha256Digest.size(), crc32Digest.size());
}

} // namespace
// ==============================================================================
// Runtime Memory Guardian
// File: tests/unit/utils/test_sha256.cpp
//
// Verifies rmg::utils::Sha256 against the well-known FIPS 180-4 / NIST test
// vectors, ensuring the from-scratch implementation is standards-compliant.
// ==============================================================================

#include <rmg/utils/sha256.hpp>
#include <rmg/utils/string_utils.hpp>

#include <gtest/gtest.h>

namespace {

using rmg::core::ByteView;
using rmg::utils::Sha256;

[[nodiscard]] ByteView asBytes(const std::string& text) {
    return ByteView(reinterpret_cast<const std::byte*>(text.data()), text.size());
}

[[nodiscard]] std::string digestToHex(const std::array<std::byte, Sha256::DIGEST_SIZE>& digest) {
    return rmg::utils::bytesToHex(ByteView(digest.data(), digest.size()));
}

TEST(Sha256Test, EmptyStringProducesKnownDigest) {
    // NIST test vector: SHA-256("") =
    // e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    const auto digest = Sha256::hash(asBytes(""));
    EXPECT_EQ(digestToHex(digest),
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(Sha256Test, AbcProducesKnownDigest) {
    // NIST test vector: SHA-256("abc") =
    // ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
    const auto digest = Sha256::hash(asBytes("abc"));
    EXPECT_EQ(digestToHex(digest),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(Sha256Test, TwoBlockMessageProducesKnownDigest) {
    // NIST test vector spanning multiple 512-bit blocks:
    // SHA-256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") =
    // 248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1
    const auto digest =
        Sha256::hash(asBytes("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"));
    EXPECT_EQ(digestToHex(digest),
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST(Sha256Test, IncrementalUpdateMatchesOneShotHash) {
    const std::string fullMessage = "The quick brown fox jumps over the lazy dog";

    const auto oneShotDigest = Sha256::hash(asBytes(fullMessage));

    Sha256 hasher;
    hasher.update(asBytes("The quick brown fox "));
    hasher.update(asBytes("jumps over the lazy dog"));
    const auto incrementalDigest = hasher.finalize();

    EXPECT_EQ(digestToHex(oneShotDigest), digestToHex(incrementalDigest));
}

TEST(Sha256Test, ResetAllowsHasherReuse) {
    Sha256 hasher;
    hasher.update(asBytes("first message"));
    const auto firstDigest = hasher.finalize();

    hasher.reset();
    hasher.update(asBytes("abc"));
    const auto secondDigest = hasher.finalize();

    EXPECT_EQ(digestToHex(secondDigest), digestToHex(Sha256::hash(asBytes("abc"))));
    EXPECT_NE(digestToHex(firstDigest), digestToHex(secondDigest));
}

TEST(Sha256Test, DifferentInputsProduceDifferentDigests) {
    const auto digestA = Sha256::hash(asBytes("input A"));
    const auto digestB = Sha256::hash(asBytes("input B"));
    EXPECT_NE(digestToHex(digestA), digestToHex(digestB));
}

TEST(Sha256Test, ByteAtATimeUpdateMatchesOneShotHash) {
    const std::string message = "byte-by-byte streaming test message payload";
    const auto oneShotDigest = Sha256::hash(asBytes(message));

    Sha256 hasher;
    for (char c : message) {
        hasher.update(ByteView(reinterpret_cast<const std::byte*>(&c), 1));
    }
    const auto streamedDigest = hasher.finalize();

    EXPECT_EQ(digestToHex(oneShotDigest), digestToHex(streamedDigest));
}

} // namespace
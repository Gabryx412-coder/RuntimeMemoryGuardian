// ==============================================================================
// Runtime Memory Guardian
// File: src/utils/sha256.cpp
//
// Reference SHA-256 implementation per FIPS 180-4.
// ==============================================================================

#include <rmg/utils/sha256.hpp>

#include <cstring>

namespace rmg::utils {

namespace {

constexpr std::array<std::uint32_t, 64> K = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U,
    0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU,
    0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU,
    0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
    0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U,
    0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U,
    0xc67178f2U,
};

constexpr std::array<std::uint32_t, 8> INITIAL_STATE = {
    0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
    0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
};

[[nodiscard]] constexpr std::uint32_t rotr(std::uint32_t value, int bits) noexcept {
    return (value >> bits) | (value << (32 - bits));
}

} // namespace

void Sha256::reset() noexcept {
    state_ = INITIAL_STATE;
    bufferLength_ = 0;
    totalLength_ = 0;
    buffer_.fill(0);
}

void Sha256::processBlock(const std::uint8_t* block) noexcept {
    std::array<std::uint32_t, 64> w{};

    for (int i = 0; i < 16; ++i) {
        w[static_cast<std::size_t>(i)] = (static_cast<std::uint32_t>(block[i * 4 + 0]) << 24) |
                                         (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
                                         (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
                                         (static_cast<std::uint32_t>(block[i * 4 + 3]));
    }

    for (int i = 16; i < 64; ++i) {
        const std::uint32_t s0 = rotr(w[static_cast<std::size_t>(i - 15)], 7) ^
                                 rotr(w[static_cast<std::size_t>(i - 15)], 18) ^
                                 (w[static_cast<std::size_t>(i - 15)] >> 3);
        const std::uint32_t s1 = rotr(w[static_cast<std::size_t>(i - 2)], 17) ^
                                 rotr(w[static_cast<std::size_t>(i - 2)], 19) ^
                                 (w[static_cast<std::size_t>(i - 2)] >> 10);
        w[static_cast<std::size_t>(i)] =
            w[static_cast<std::size_t>(i - 16)] + s0 + w[static_cast<std::size_t>(i - 7)] + s1;
    }

    std::uint32_t a = state_[0];
    std::uint32_t b = state_[1];
    std::uint32_t c = state_[2];
    std::uint32_t d = state_[3];
    std::uint32_t e = state_[4];
    std::uint32_t f = state_[5];
    std::uint32_t g = state_[6];
    std::uint32_t h = state_[7];

    for (int i = 0; i < 64; ++i) {
        const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const std::uint32_t ch = (e & f) ^ ((~e) & g);
        const std::uint32_t temp1 =
            h + s1 + ch + K[static_cast<std::size_t>(i)] + w[static_cast<std::size_t>(i)];
        const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temp2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
}

void Sha256::update(rmg::core::ByteView data) noexcept {
    const auto* input = reinterpret_cast<const std::uint8_t*>(data.data());
    std::size_t remaining = data.size();
    totalLength_ += remaining;

    // Fill any partially-filled buffer from a previous call first.
    if (bufferLength_ > 0) {
        const std::size_t needed = BLOCK_SIZE - bufferLength_;
        const std::size_t toCopy = (remaining < needed) ? remaining : needed;
        std::memcpy(buffer_.data() + bufferLength_, input, toCopy);
        bufferLength_ += toCopy;
        input += toCopy;
        remaining -= toCopy;

        if (bufferLength_ == BLOCK_SIZE) {
            processBlock(buffer_.data());
            bufferLength_ = 0;
        }
    }

    // Process full blocks directly from the input buffer.
    while (remaining >= BLOCK_SIZE) {
        processBlock(input);
        input += BLOCK_SIZE;
        remaining -= BLOCK_SIZE;
    }

    // Buffer any leftover partial block for the next call.
    if (remaining > 0) {
        std::memcpy(buffer_.data(), input, remaining);
        bufferLength_ = remaining;
    }
}

std::array<std::byte, Sha256::DIGEST_SIZE> Sha256::finalize() noexcept {
    const std::uint64_t bitLength = totalLength_ * 8;

    // Append the mandatory 0x80 padding byte.
    std::uint8_t padByte = 0x80;
    update(rmg::core::ByteView{reinterpret_cast<const std::byte*>(&padByte), 1});

    // Undo the totalLength_ increment caused by the padding byte itself,
    // since totalLength_ must reflect only the original message length.
    totalLength_ -= 1;

    // Pad with zeros until the buffer holds exactly 56 bytes (mod 64),
    // leaving room for the 8-byte big-endian length field.
    static constexpr std::uint8_t ZERO = 0;
    while (bufferLength_ != 56) {
        update(rmg::core::ByteView{reinterpret_cast<const std::byte*>(&ZERO), 1});
        totalLength_ -= 1;
    }

    std::array<std::uint8_t, 8> lengthBytes{};
    for (int i = 0; i < 8; ++i) {
        lengthBytes[static_cast<std::size_t>(i)] =
            static_cast<std::uint8_t>(bitLength >> (56 - 8 * i));
    }
    std::memcpy(buffer_.data() + 56, lengthBytes.data(), 8);
    processBlock(buffer_.data());

    std::array<std::byte, DIGEST_SIZE> digest{};
    for (int i = 0; i < 8; ++i) {
        const std::uint32_t word = state_[static_cast<std::size_t>(i)];
        digest[static_cast<std::size_t>(i * 4 + 0)] = static_cast<std::byte>(word >> 24);
        digest[static_cast<std::size_t>(i * 4 + 1)] = static_cast<std::byte>(word >> 16);
        digest[static_cast<std::size_t>(i * 4 + 2)] = static_cast<std::byte>(word >> 8);
        digest[static_cast<std::size_t>(i * 4 + 3)] = static_cast<std::byte>(word);
    }
    return digest;
}

std::array<std::byte, Sha256::DIGEST_SIZE> Sha256::hash(rmg::core::ByteView data) noexcept {
    Sha256 hasher;
    hasher.update(data);
    return hasher.finalize();
}

} // namespace rmg::utils
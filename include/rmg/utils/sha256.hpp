// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/utils/sha256.hpp
//
// Self-contained SHA-256 implementation (FIPS 180-4). No external
// dependencies are used so that RMG's integrity baselines remain verifiable
// without pulling in a third-party crypto library. This is a defensive
// utility used purely to detect memory tampering — it is not exposed as a
// general-purpose cryptographic API and makes no security claims beyond
// "produces the standard SHA-256 digest of the given bytes".
// ==============================================================================

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <rmg/core/types.hpp>

namespace rmg::utils {

/// @brief Incremental SHA-256 hasher.
///
/// Usage:
/// @code
/// Sha256 hasher;
/// hasher.update(chunk1);
/// hasher.update(chunk2);
/// auto digest = hasher.finalize(); // std::array<std::byte, 32>
/// @endcode
///
/// Calling update() after finalize() without calling reset() first is
/// undefined behavior from the caller's perspective (state has already been
/// padded and is no longer a valid running hash).
class Sha256 {
public:
    static constexpr std::size_t DIGEST_SIZE = 32;
    static constexpr std::size_t BLOCK_SIZE = 64;

    Sha256() { reset(); }

    /// @brief Resets the hasher to its initial state, discarding any data
    ///        previously fed via update().
    void reset() noexcept;

    /// @brief Feeds additional bytes into the running hash computation.
    void update(rmg::core::ByteView data) noexcept;

    /// @brief Finalizes the hash and returns the 32-byte digest.
    ///
    /// @note This applies padding and processes the final block(s). The
    ///       hasher must be reset() before it can be reused for a new
    ///       computation.
    [[nodiscard]] std::array<std::byte, DIGEST_SIZE> finalize() noexcept;

    /// @brief One-shot convenience helper equivalent to constructing a
    ///        Sha256, calling update(data) once, then finalize().
    [[nodiscard]] static std::array<std::byte, DIGEST_SIZE> hash(rmg::core::ByteView data) noexcept;

private:
    void processBlock(const std::uint8_t* block) noexcept;

    std::array<std::uint32_t, 8> state_{};
    std::array<std::uint8_t, BLOCK_SIZE> buffer_{};
    std::size_t bufferLength_ = 0;
    std::uint64_t totalLength_ = 0;
};

} // namespace rmg::utils
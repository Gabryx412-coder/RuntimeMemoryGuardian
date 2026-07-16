// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/integrity/hash_provider.hpp
//
// Abstracts the digest algorithm used to fingerprint code sections for
// integrity comparison, decoupling IntegrityBaseline/IntegrityChecker from
// any specific algorithm. Two concrete providers are supplied out of the
// box: a cryptographic one (SHA-256, the recommended default) and a fast,
// non-cryptographic one (CRC32) for scenarios prioritizing throughput over
// tamper-resistance of the digest itself.
// ==============================================================================

#pragma once

#include <memory>
#include <vector>

#include <rmg/core/types.hpp>

namespace rmg::integrity {

/// @brief A digest value: an ordered sequence of bytes whose length is
///        determined by the producing IHashProvider.
using Digest = std::vector<std::byte>;

/// @brief Abstract interface for computing digests over byte ranges.
class IHashProvider {
public:
    virtual ~IHashProvider() = default;

    /// @brief Computes the digest of @p data according to this provider's
    ///        algorithm.
    [[nodiscard]] virtual Digest hash(rmg::core::ByteView data) const = 0;

    /// @brief A short, stable identifier for the algorithm (e.g. "SHA-256",
    ///        "CRC32"), used when serializing baselines so a stored digest
    ///        can be validated against the provider used to reproduce it.
    [[nodiscard]] virtual std::string_view algorithmName() const noexcept = 0;
};

/// @brief Cryptographic hash provider backed by rmg::utils::Sha256.
///
/// This is the recommended provider for integrity baselines: SHA-256 makes
/// it computationally infeasible for an attacker to modify code while
/// preserving the original digest.
class Sha256HashProvider final : public IHashProvider {
public:
    [[nodiscard]] Digest hash(rmg::core::ByteView data) const override;
    [[nodiscard]] std::string_view algorithmName() const noexcept override;
};

/// @brief Fast, non-cryptographic hash provider backed by
///        rmg::utils::crc32.
///
/// @warning CRC32 provides no resistance against a deliberate adversary who
///          can compute a colliding patch. Use only for cheap
///          change-detection heuristics (e.g. a fast first-pass filter
///          before falling back to Sha256HashProvider), never as the sole
///          basis of a security-critical integrity guarantee.
class Crc32HashProvider final : public IHashProvider {
public:
    [[nodiscard]] Digest hash(rmg::core::ByteView data) const override;
    [[nodiscard]] std::string_view algorithmName() const noexcept override;
};

/// @brief Constructs the default recommended hash provider (SHA-256).
[[nodiscard]] std::unique_ptr<IHashProvider> makeDefaultHashProvider();

} // namespace rmg::integrity
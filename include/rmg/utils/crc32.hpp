// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/utils/crc32.hpp
//
// Fast, non-cryptographic CRC-32 (IEEE 802.3 polynomial) checksum, used where
// raw throughput matters more than cryptographic strength — e.g. a cheap
// first-pass filter before an expensive SHA-256 comparison, or high-frequency
// polling scenarios in ProcessMonitor.
//
// This is NOT a substitute for SHA-256 in integrity-critical baselines: CRC32
// is trivially forgeable by an adversary who can compute collisions. It is
// intended purely as a fast change-detection heuristic.
// ==============================================================================

#pragma once

#include <rmg/core/types.hpp>

#include <cstdint>

namespace rmg::utils {

/// @brief Computes the CRC-32 (IEEE 802.3) checksum of @p data.
[[nodiscard]] std::uint32_t crc32(rmg::core::ByteView data) noexcept;

} // namespace rmg::utils
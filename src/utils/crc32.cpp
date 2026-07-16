// ==============================================================================
// Runtime Memory Guardian
// File: src/utils/crc32.cpp
// ==============================================================================

#include <rmg/utils/crc32.hpp>

#include <array>

namespace rmg::utils {

namespace {

[[nodiscard]] constexpr std::array<std::uint32_t, 256> buildTable() noexcept {
    std::array<std::uint32_t, 256> table{};
    constexpr std::uint32_t POLYNOMIAL = 0xEDB88320U;

    for (std::uint32_t i = 0; i < 256; ++i) {
        std::uint32_t c = i;
        for (int bit = 0; bit < 8; ++bit) {
            c = (c & 1U) ? (POLYNOMIAL ^ (c >> 1)) : (c >> 1);
        }
        table[i] = c;
    }
    return table;
}

constexpr std::array<std::uint32_t, 256> CRC_TABLE = buildTable();

} // namespace

std::uint32_t crc32(rmg::core::ByteView data) noexcept {
    std::uint32_t crc = 0xFFFFFFFFU;

    for (const std::byte b : data) {
        const auto byteValue = static_cast<std::uint8_t>(b);
        const std::uint8_t index = static_cast<std::uint8_t>((crc ^ byteValue) & 0xFFU);
        crc = CRC_TABLE[index] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFFU;
}

} // namespace rmg::utils
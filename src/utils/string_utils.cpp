// ==============================================================================
// Runtime Memory Guardian
// File: src/utils/string_utils.cpp
// ==============================================================================

#include <rmg/utils/string_utils.hpp>

#include <cctype>
#include <cstdio>

namespace rmg::utils {

namespace {

[[nodiscard]] char toLowerAscii(char c) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

} // namespace

std::string toHex(rmg::core::Address address) {
    char buffer[2 + sizeof(rmg::core::Address) * 2 + 1] = {};
    std::snprintf(buffer, sizeof(buffer), "0x%0*zx",
                  static_cast<int>(sizeof(rmg::core::Address) * 2),
                  static_cast<std::size_t>(address));
    return std::string(buffer);
}

std::string bytesToHex(rmg::core::ByteView bytes) {
    static constexpr char HEX_DIGITS[] = "0123456789abcdef";

    std::string result;
    result.reserve(bytes.size() * 2);

    for (const std::byte b : bytes) {
        const auto value = static_cast<std::uint8_t>(b);
        result.push_back(HEX_DIGITS[(value >> 4) & 0x0FU]);
        result.push_back(HEX_DIGITS[value & 0x0FU]);
    }

    return result;
}

bool iequals(std::string_view lhs, std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (toLowerAscii(lhs[i]) != toLowerAscii(rhs[i])) {
            return false;
        }
    }
    return true;
}

bool iEndsWith(std::string_view text, std::string_view suffix) noexcept {
    if (suffix.size() > text.size()) {
        return false;
    }
    return iequals(text.substr(text.size() - suffix.size()), suffix);
}

} // namespace rmg::utils
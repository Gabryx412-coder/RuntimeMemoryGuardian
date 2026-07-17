// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/utils/string_utils.hpp
//
// Small, dependency-free string formatting/comparison helpers used across
// logging, module-name matching (case-insensitive on Windows), and the
// rmg-cli tool's human-readable output.
// ==============================================================================

#pragma once

#include <rmg/core/types.hpp>

#include <string>
#include <string_view>

namespace rmg::utils {

/// @brief Formats @p address as a zero-padded hexadecimal string prefixed
///        with "0x", e.g. 0x00007ffe12340000.
[[nodiscard]] std::string toHex(rmg::core::Address address);

/// @brief Formats an arbitrary byte buffer as a lowercase hexadecimal
///        string with no separators, e.g. bytes {0xDE, 0xAD} -> "dead".
[[nodiscard]] std::string bytesToHex(rmg::core::ByteView bytes);

/// @brief Case-insensitive ASCII string equality check.
///
/// Used primarily for matching module/file names, which are
/// case-insensitive on Windows (NTFS/PE loader semantics) even though RMG
/// itself is cross-platform.
[[nodiscard]] bool iequals(std::string_view lhs, std::string_view rhs) noexcept;

/// @brief Returns true if @p text ends with @p suffix, comparing
///        case-insensitively (e.g. matching ".dll" or ".so" extensions).
[[nodiscard]] bool iEndsWith(std::string_view text, std::string_view suffix) noexcept;

} // namespace rmg::utils
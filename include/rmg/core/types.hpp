// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/core/types.hpp
//
// Fundamental, platform-neutral types shared by every subsystem. Keeping
// these aliases centralized avoids subtle mismatches (e.g. one module using
// uint64_t and another using uintptr_t for the same conceptual "address").
// ==============================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace rmg::core {

/// @brief A memory address within a (possibly remote) process' address space.
///
/// Deliberately an alias rather than a strong type: addresses are frequently
/// subject to arithmetic (offsetting, range checks) and wrapping them in a
/// class would add friction without meaningful safety benefit at this layer.
using Address = std::uintptr_t;

/// @brief A read-only, non-owning view over a contiguous range of bytes.
using ByteView = std::span<const std::byte>;

/// @brief A mutable, non-owning view over a contiguous range of bytes.
using MutableByteView = std::span<std::byte>;

/// @brief Identifies an operating-system process.
///
/// On Windows this corresponds to a DWORD process id; on Linux, to a pid_t.
/// Both fit comfortably in a std::uint32_t for all practical purposes.
using ProcessId = std::uint32_t;

/// @brief Memory protection flags for a mapped region, expressed in a
///        platform-neutral form.
///
/// The values are a bitmask so that a region can be simultaneously e.g.
/// Read | Execute. Platform layers are responsible for translating to/from
/// native protection constants (PAGE_EXECUTE_READ, PROT_READ|PROT_EXEC, ...).
enum class MemoryProtection : std::uint8_t {
    None = 0,
    Read = 1U << 0,
    Write = 1U << 1,
    Execute = 1U << 2,
    Guard = 1U << 3,
};

[[nodiscard]] constexpr MemoryProtection operator|(MemoryProtection lhs, MemoryProtection rhs) noexcept {
    return static_cast<MemoryProtection>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

[[nodiscard]] constexpr MemoryProtection operator&(MemoryProtection lhs, MemoryProtection rhs) noexcept {
    return static_cast<MemoryProtection>(static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
}

[[nodiscard]] constexpr bool hasFlag(MemoryProtection value, MemoryProtection flag) noexcept {
    return static_cast<std::uint8_t>(value & flag) != 0;
}

/// @brief Human-readable rendering of a MemoryProtection value, e.g. "RWX".
///
/// Order is fixed as Read, Write, Execute, Guard; absent flags are omitted.
/// Returns "---" if no flags are set.
[[nodiscard]] std::string toString(MemoryProtection protection);

} // namespace rmg::core
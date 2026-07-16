// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/platform/native_types.hpp
//
// Isolates OS-native handle/identifier types behind platform-neutral aliases,
// so that public headers never need to #include <windows.h> or <unistd.h>
// directly. Concrete platform backends (src/platform/windows,
// src/platform/linux) are responsible for casting to/from the real native
// types.
// ==============================================================================

#pragma once

#include <cstdint>

namespace rmg::platform {

#if defined(RMG_PLATFORM_WINDOWS)

/// @brief Opaque native process/module handle.
///
/// On Windows this is a `HANDLE` (i.e. `void*`) in disguise; on Linux it is
/// unused for process identity (Linux uses NativeProcessId instead) but is
/// kept for symmetry and for potential future use (e.g. memory-mapped file
/// descriptors surfaced as a "handle").
using NativeHandle = void*;

/// @brief Sentinel value representing "no handle" / an invalid handle.
inline constexpr NativeHandle INVALID_NATIVE_HANDLE = nullptr;

#elif defined(RMG_PLATFORM_LINUX)

/// @brief On Linux, "handles" in the RMG sense are plain file descriptors
///        (e.g. an open /proc/[pid]/mem fd).
using NativeHandle = int;

inline constexpr NativeHandle INVALID_NATIVE_HANDLE = -1;

#else
#error "Runtime Memory Guardian: unsupported platform (native_types.hpp)"
#endif

/// @brief Native operating-system process identifier.
///
/// Windows DWORD and Linux pid_t both fit within a 32-bit unsigned value;
/// this alias avoids leaking either OS' typedef into public headers.
using NativeProcessId = std::uint32_t;

} // namespace rmg::platform
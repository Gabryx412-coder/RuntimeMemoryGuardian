// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/platform/process_handle.hpp
//
// RAII wrapper around a native operating-system process handle/reference.
// Concrete construction (OpenProcess / opening /proc/[pid]/mem) is
// implemented per-platform in src/platform/{windows,linux}; this header
// defines only the cross-platform surface that the rest of RMG depends on.
// ==============================================================================

#pragma once

#include <rmg/core/error.hpp>
#include <rmg/core/non_copyable.hpp>
#include <rmg/platform/native_types.hpp>

namespace rmg::platform {

/// @brief Owns a reference to a target operating-system process, granting
///        the access rights RMG needs (memory read, and query-only
///        information) for the lifetime of the object.
///
/// ProcessHandle is move-only: copying a native OS handle would either
/// require duplicating it (extra syscalls, extra failure modes) or risk a
/// double-close, so RMG makes ownership explicit and singular.
class ProcessHandle final : private rmg::core::NonCopyable {
public:
    /// @brief Opens a handle to the process identified by @p processId, with
    ///        the minimal access rights required for memory inspection
    ///        (read + query, never write or execute).
    ///
    /// @return A valid ProcessHandle on success, or an Error describing why
    ///         the process could not be opened (e.g. AccessDenied if the
    ///         calling process lacks sufficient privileges, NotFound if no
    ///         such process id exists).
    [[nodiscard]] static rmg::core::Result<ProcessHandle> open(NativeProcessId processId);

    /// @brief Constructs a ProcessHandle representing the currently running
    ///        process ("self"). Useful for self-integrity checks where the
    ///        host application monitors its own memory.
    [[nodiscard]] static rmg::core::Result<ProcessHandle> openSelf();

    ProcessHandle(ProcessHandle&& other) noexcept;
    ProcessHandle& operator=(ProcessHandle&& other) noexcept;
    ~ProcessHandle();

    /// @brief Returns the OS-native handle/descriptor owned by this object.
    ///
    /// Intended for consumption by the platform-specific backend
    /// implementations (e.g. WindowsPlatformTraits); application code
    /// should not normally need to touch this.
    [[nodiscard]] NativeHandle nativeHandle() const noexcept { return handle_; }

    /// @brief The process id this handle refers to.
    [[nodiscard]] NativeProcessId processId() const noexcept { return processId_; }

    /// @brief True if this object owns a valid, open native handle.
    [[nodiscard]] bool isValid() const noexcept { return handle_ != INVALID_NATIVE_HANDLE; }

private:
    ProcessHandle(NativeHandle handle, NativeProcessId processId) noexcept
        : handle_(handle), processId_(processId) {}

    void reset() noexcept;

    NativeHandle handle_ = INVALID_NATIVE_HANDLE;
    NativeProcessId processId_ = 0;
};

} // namespace rmg::platform
// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/linux/linux_process_handle.cpp
//
// Implements rmg::platform::ProcessHandle for Linux, plus the shared helper
// functions declared in linux_process_handle.hpp (errno formatting,
// permission-string parsing, and the pread()-based memory reader).
//
// RMG opens /proc/[pid]/mem once per ProcessHandle and keeps the descriptor
// for the handle's lifetime, avoiding repeated open()/close() overhead on
// every readMemory() call.
// ==============================================================================

#include "linux_process_handle.hpp"

#include <cerrno>
#include <cstring>
#include <string>

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

namespace rmg::platform {

// ------------------------------------------------------------------------------
// ProcessHandle
// ------------------------------------------------------------------------------

rmg::core::Result<ProcessHandle> ProcessHandle::open(NativeProcessId processId) {
    // Verify the process exists and is signalable by us before attempting
    // to open /proc/[pid]/mem, so we can distinguish "no such process" from
    // "exists but access denied" with a clearer error.
    if (::kill(static_cast<pid_t>(processId), 0) != 0) {
        if (errno == ESRCH) {
            return rmg::core::fail<ProcessHandle>(
                rmg::core::ErrorCode::NotFound,
                "no such process id " + std::to_string(processId));
        }
        if (errno == EPERM) {
            return rmg::core::fail<ProcessHandle>(
                rmg::core::ErrorCode::AccessDenied,
                "insufficient privileges to signal/query process " + std::to_string(processId));
        }
        // Any other errno: fall through and let the open() below produce
        // the authoritative failure reason.
    }

    const std::string memPath = "/proc/" + std::to_string(processId) + "/mem";
    const int fd = ::open(memPath.c_str(), O_RDONLY);

    if (fd < 0) {
        const int savedErrno = errno;
        if (savedErrno == EACCES || savedErrno == EPERM) {
            return rmg::core::fail<ProcessHandle>(
                rmg::core::ErrorCode::AccessDenied,
                "open(" + memPath + ") failed: " + detail::errnoToString(savedErrno));
        }
        if (savedErrno == ENOENT) {
            return rmg::core::fail<ProcessHandle>(
                rmg::core::ErrorCode::NotFound,
                "process " + std::to_string(processId) + " no longer exists");
        }
        return rmg::core::fail<ProcessHandle>(
            rmg::core::ErrorCode::PlatformError,
            "open(" + memPath + ") failed: " + detail::errnoToString(savedErrno));
    }

    return ProcessHandle(fd, processId);
}

rmg::core::Result<ProcessHandle> ProcessHandle::openSelf() {
    return open(static_cast<NativeProcessId>(::getpid()));
}

ProcessHandle::ProcessHandle(ProcessHandle&& other) noexcept
    : handle_(other.handle_), processId_(other.processId_) {
    other.handle_ = INVALID_NATIVE_HANDLE;
    other.processId_ = 0;
}

ProcessHandle& ProcessHandle::operator=(ProcessHandle&& other) noexcept {
    if (this != &other) {
        reset();
        handle_ = other.handle_;
        processId_ = other.processId_;
        other.handle_ = INVALID_NATIVE_HANDLE;
        other.processId_ = 0;
    }
    return *this;
}

ProcessHandle::~ProcessHandle() {
    reset();
}

void ProcessHandle::reset() noexcept {
    if (handle_ != INVALID_NATIVE_HANDLE) {
        ::close(handle_);
        handle_ = INVALID_NATIVE_HANDLE;
    }
}

// ------------------------------------------------------------------------------
// detail helpers
// ------------------------------------------------------------------------------

namespace detail {

rmg::core::MemoryProtection parseProtectionString(std::string_view permissions) noexcept {
    using rmg::core::MemoryProtection;

    // Expected format (from /proc/[pid]/maps): "rwxp" or "r-xp", etc.
    // Index: 0 = read, 1 = write, 2 = execute, 3 = shared/private flag (ignored).
    if (permissions.size() < 3) {
        return MemoryProtection::None;
    }

    MemoryProtection result = MemoryProtection::None;
    if (permissions[0] == 'r') {
        result = result | MemoryProtection::Read;
    }
    if (permissions[1] == 'w') {
        result = result | MemoryProtection::Write;
    }
    if (permissions[2] == 'x') {
        result = result | MemoryProtection::Execute;
    }
    return result;
}

std::string errnoToString(int errnoValue) {
    std::array<char, 256> buffer{};

#if defined(_GNU_SOURCE) && !defined(__APPLE__)
    // GNU-specific strerror_r returns char*, which may or may not be
    // buffer.data() depending on glibc internals.
    const char* message = ::strerror_r(errnoValue, buffer.data(), buffer.size());
    std::string text(message != nullptr ? message : "unknown error");
#else
    // POSIX strerror_r returns an int status code and always writes into
    // the supplied buffer on success.
    const int status = ::strerror_r(errnoValue, buffer.data(), buffer.size());
    std::string text(status == 0 ? buffer.data() : "unknown error");
#endif

    return "[" + std::to_string(errnoValue) + "] " + text;
}

rmg::core::Result<std::size_t>
readMemoryLinux(const ProcessHandle& handle,
                 rmg::core::Address address,
                 rmg::core::MutableByteView destination) {
    const ssize_t bytesRead = ::pread(
        handle.nativeHandle(),
        destination.data(),
        destination.size(),
        static_cast<off_t>(address));

    if (bytesRead < 0) {
        return rmg::core::fail<std::size_t>(
            rmg::core::ErrorCode::MemoryAccessFailure,
            "pread(/proc/" + std::to_string(handle.processId()) + "/mem) failed at " +
                std::to_string(address) + ": " + errnoToString(errno));
    }

    // pread() returning 0 here means we hit the end of a readable region;
    // treat as a valid (possibly partial or empty) result rather than an error.
    return static_cast<std::size_t>(bytesRead);
}

} // namespace detail

} // namespace rmg::platform
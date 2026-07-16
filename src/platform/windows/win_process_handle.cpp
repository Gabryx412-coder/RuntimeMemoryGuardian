// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/windows/win_process_handle.cpp
//
// Implements rmg::platform::ProcessHandle for Windows, plus the shared
// helper functions declared in win_process_handle.hpp (handle conversion,
// protection translation, error formatting, ReadProcessMemory wrapper).
// ==============================================================================

#include "win_process_handle.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace rmg::platform {

// ------------------------------------------------------------------------------
// ProcessHandle
// ------------------------------------------------------------------------------

rmg::core::Result<ProcessHandle> ProcessHandle::open(NativeProcessId processId) {
    constexpr DWORD REQUIRED_ACCESS =
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_OPERATION;

    HANDLE rawHandle = ::OpenProcess(REQUIRED_ACCESS, FALSE, static_cast<DWORD>(processId));
    if (rawHandle == nullptr) {
        const DWORD lastError = ::GetLastError();
        if (lastError == ERROR_INVALID_PARAMETER) {
            return rmg::core::fail<ProcessHandle>(
                rmg::core::ErrorCode::NotFound,
                "OpenProcess: no such process id " + std::to_string(processId));
        }
        return rmg::core::fail<ProcessHandle>(
            rmg::core::ErrorCode::AccessDenied,
            "OpenProcess failed: " + detail::lastErrorToString(lastError));
    }

    return ProcessHandle(detail::fromWinHandle(rawHandle), processId);
}

rmg::core::Result<ProcessHandle> ProcessHandle::openSelf() {
    // GetCurrentProcess() returns a pseudo-handle that does not need to be
    // (and must not be) closed via CloseHandle, unlike a real OpenProcess
    // handle. We special-case this by duplicating it into a real,
    // independently-closable handle so ProcessHandle's destructor logic
    // stays uniform regardless of how the handle was obtained.
    HANDLE pseudoHandle = ::GetCurrentProcess();
    HANDLE realHandle = nullptr;

    const BOOL duplicated = ::DuplicateHandle(
        pseudoHandle, pseudoHandle,
        pseudoHandle, &realHandle,
        0, FALSE, DUPLICATE_SAME_ACCESS);

    if (duplicated == 0 || realHandle == nullptr) {
        return rmg::core::fail<ProcessHandle>(
            rmg::core::ErrorCode::PlatformError,
            "DuplicateHandle(self) failed: " + detail::lastErrorToString(::GetLastError()));
    }

    return ProcessHandle(detail::fromWinHandle(realHandle),
                          static_cast<NativeProcessId>(::GetCurrentProcessId()));
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
        ::CloseHandle(detail::toWinHandle(handle_));
        handle_ = INVALID_NATIVE_HANDLE;
    }
}

// ------------------------------------------------------------------------------
// detail helpers
// ------------------------------------------------------------------------------

namespace detail {

void* toWinHandle(NativeHandle handle) noexcept {
    return handle;
}

NativeHandle fromWinHandle(void* winHandle) noexcept {
    return winHandle;
}

rmg::core::MemoryProtection toRmgProtection(std::uint32_t winProtect) noexcept {
    using rmg::core::MemoryProtection;

    // Mask off modifier bits (PAGE_GUARD, PAGE_NOCACHE, PAGE_WRITECOMBINE)
    // which are handled separately below.
    const std::uint32_t base = winProtect & 0xFFU;

    MemoryProtection result = MemoryProtection::None;

    switch (base) {
        case PAGE_READONLY:
            result = MemoryProtection::Read;
            break;
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
            result = MemoryProtection::Read | MemoryProtection::Write;
            break;
        case PAGE_EXECUTE:
            result = MemoryProtection::Execute;
            break;
        case PAGE_EXECUTE_READ:
            result = MemoryProtection::Read | MemoryProtection::Execute;
            break;
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            result = MemoryProtection::Read | MemoryProtection::Write | MemoryProtection::Execute;
            break;
        case PAGE_NOACCESS:
        default:
            result = MemoryProtection::None;
            break;
    }

    if ((winProtect & PAGE_GUARD) != 0U) {
        result = result | MemoryProtection::Guard;
    }

    return result;
}

std::string lastErrorToString(std::uint32_t errorCode) {
    LPSTR messageBuffer = nullptr;

    const DWORD length = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&messageBuffer),
        0,
        nullptr);

    std::string message;
    if (length > 0 && messageBuffer != nullptr) {
        message.assign(messageBuffer, length);
        // Trim trailing CR/LF that FormatMessageA typically appends.
        while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
            message.pop_back();
        }
    } else {
        message = "unknown error";
    }

    if (messageBuffer != nullptr) {
        ::LocalFree(messageBuffer);
    }

    return "[" + std::to_string(errorCode) + "] " + message;
}

rmg::core::Result<std::size_t>
readMemoryWindows(const ProcessHandle& handle,
                   rmg::core::Address address,
                   rmg::core::MutableByteView destination) {
    SIZE_T bytesRead = 0;

    const BOOL succeeded = ::ReadProcessMemory(
        toWinHandle(handle.nativeHandle()),
        reinterpret_cast<LPCVOID>(address),
        destination.data(),
        destination.size(),
        &bytesRead);

    if (succeeded == 0 && bytesRead == 0) {
        return rmg::core::fail<std::size_t>(
            rmg::core::ErrorCode::MemoryAccessFailure,
            "ReadProcessMemory failed at " + std::to_string(address) + ": " +
                lastErrorToString(::GetLastError()));
    }

    // A partial read (succeeded == FALSE but bytesRead > 0, or succeeded ==
    // TRUE with bytesRead < requested near a region boundary) is reported
    // as a successful partial result rather than an error.
    return static_cast<std::size_t>(bytesRead);
}

rmg::core::Result<rmg::core::MemoryProtection>
queryProtectionWindows(const ProcessHandle& handle, rmg::core::Address address) {
    MEMORY_BASIC_INFORMATION info{};

    const SIZE_T written = ::VirtualQueryEx(
        toWinHandle(handle.nativeHandle()),
        reinterpret_cast<LPCVOID>(address),
        &info,
        sizeof(info));

    if (written == 0) {
        return rmg::core::fail<rmg::core::MemoryProtection>(
            rmg::core::ErrorCode::RegionNotFound,
            "VirtualQueryEx failed at " + std::to_string(address) + ": " +
                lastErrorToString(::GetLastError()));
    }

    if (info.State != MEM_COMMIT) {
        return rmg::core::fail<rmg::core::MemoryProtection>(
            rmg::core::ErrorCode::RegionNotFound,
            "address " + std::to_string(address) + " is not committed memory");
    }

    return toRmgProtection(static_cast<std::uint32_t>(info.Protect));
}

} // namespace detail

} // namespace rmg::platform
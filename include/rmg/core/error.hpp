// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/core/error.hpp
//
// Uniform, exception-free error handling used across the entire library.
// All fallible operations in RMG return an rmg::core::Result<T> instead of
// throwing, which keeps the hot paths (memory scanning, hook detection)
// predictable and allocation-free on the error path.
// ==============================================================================

#pragma once

#include <expected>
#include <string>
#include <string_view>

namespace rmg::core {

/// @brief Enumerates every category of failure that can occur inside RMG.
///
/// The list is intentionally flat rather than hierarchical: callers are
/// expected to switch on this enum, and a flat set of well-documented values
/// is easier to reason about than a taxonomy of exception classes.
enum class ErrorCode {
    /// Unspecified failure; used only as a last resort.
    Unknown = 0,

    /// The calling process does not have sufficient privileges to perform
    /// the requested operation (e.g. reading another process' memory).
    AccessDenied,

    /// A process, module, or memory region that was expected to exist could
    /// not be found (e.g. the target process has already exited).
    NotFound,

    /// The requested memory region is not currently mapped/valid.
    RegionNotFound,

    /// An argument passed to the API violates its documented contract.
    InvalidArgument,

    /// The underlying operating system call failed; see the platform-specific
    /// logging output for the native error code.
    PlatformError,

    /// A read or write to a memory address failed (e.g. partial read).
    MemoryAccessFailure,

    /// The requested feature is not implemented on the current platform.
    NotSupported,

    /// A baseline or snapshot could not be (de)serialized due to malformed
    /// or incompatible data.
    SerializationError,

    /// An internal invariant was violated; indicates a bug in RMG itself
    /// rather than a misuse by the caller.
    InternalError,
};

/// @brief Returns a short, human-readable, stable description of @p code.
///
/// The returned string is suitable for logging and diagnostics. It is not
/// localized and its exact wording may change between releases; callers
/// should not pattern-match on it programmatically — switch on ErrorCode
/// instead.
[[nodiscard]] std::string_view toString(ErrorCode code) noexcept;

/// @brief Rich error object carrying both a machine-readable code and an
///        optional human-readable context message.
///
/// Kept intentionally small and copyable so it can be stored inside
/// std::expected without incurring heap allocations in the common case
/// (short string optimization covers most context messages).
class Error {
public:
    Error() = default;

    explicit Error(ErrorCode code) noexcept : code_(code) {}

    Error(ErrorCode code, std::string context) : code_(code), context_(std::move(context)) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }

    [[nodiscard]] const std::string& context() const noexcept { return context_; }

    /// @brief Produces a single-line diagnostic string combining the error
    ///        code description and, if present, the context message.
    [[nodiscard]] std::string toDiagnosticString() const;

private:
    ErrorCode code_ = ErrorCode::Unknown;
    std::string context_;
};

/// @brief Standard fallible-result alias used throughout the public API.
///
/// @tparam T The success payload type. Use `Result<void>` for operations
///           that either succeed or fail without producing a value.
template <typename T>
using Result = std::expected<T, Error>;

/// @brief Convenience factory for constructing an unexpected Result<T> from
///        an ErrorCode and optional context, without repeating
///        `std::unexpected` boilerplate at every call site.
template <typename T = void>
[[nodiscard]] inline std::unexpected<Error> fail(ErrorCode code, std::string context = {}) {
    return std::unexpected<Error>(Error(code, std::move(context)));
}

} // namespace rmg::core
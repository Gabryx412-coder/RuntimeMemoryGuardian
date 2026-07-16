// ==============================================================================
// Runtime Memory Guardian
// File: src/core/error.cpp
// ==============================================================================

#include <rmg/core/error.hpp>

namespace rmg::core {

std::string_view toString(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Unknown:            return "Unknown";
        case ErrorCode::AccessDenied:        return "AccessDenied";
        case ErrorCode::NotFound:            return "NotFound";
        case ErrorCode::RegionNotFound:      return "RegionNotFound";
        case ErrorCode::InvalidArgument:     return "InvalidArgument";
        case ErrorCode::PlatformError:       return "PlatformError";
        case ErrorCode::MemoryAccessFailure: return "MemoryAccessFailure";
        case ErrorCode::NotSupported:        return "NotSupported";
        case ErrorCode::SerializationError:  return "SerializationError";
        case ErrorCode::InternalError:       return "InternalError";
    }
    return "Unrecognized";
}

std::string Error::toDiagnosticString() const {
    std::string result{toString(code_)};
    if (!context_.empty()) {
        result += ": ";
        result += context_;
    }
    return result;
}

} // namespace rmg::core
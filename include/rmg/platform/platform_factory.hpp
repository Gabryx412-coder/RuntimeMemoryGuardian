// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/platform/platform_factory.hpp
//
// Factory function that hides the concrete IPlatformTraits implementation
// selection behind a single call, so callers never need conditional
// compilation of their own to obtain a working platform backend.
// ==============================================================================

#pragma once

#include <rmg/platform/platform_traits.hpp>

#include <memory>

namespace rmg::platform {

/// @brief Creates the IPlatformTraits implementation appropriate for the
///        operating system RMG was compiled for.
///
/// @return A non-null, fully-initialized platform backend. Construction of
///         the backend itself cannot fail (any fallible initialization is
///         deferred to the individual IPlatformTraits methods, which return
///         rmg::core::Result).
[[nodiscard]] std::unique_ptr<IPlatformTraits> createPlatformTraits();

} // namespace rmg::platform
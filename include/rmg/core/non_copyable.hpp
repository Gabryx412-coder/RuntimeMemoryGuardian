// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/core/non_copyable.hpp
//
// Lightweight mixins expressing move-only / immovable ownership semantics for
// classes that wrap native resources (process handles, memory mappings...).
// Preferred over hand-rolling deleted special member functions in every class
// so the intent is visible at a glance in the class declaration.
// ==============================================================================

#pragma once

namespace rmg::core {

/// @brief Inherit from this to make a derived class non-copyable while still
///        allowing it to be movable (the derived class must declare its own
///        move constructor/assignment if it owns movable resources).
class NonCopyable {
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(NonCopyable&&) = default;

public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

/// @brief Inherit from this to make a derived class neither copyable nor
///        movable. Suitable for RAII types wrapping a resource whose
///        identity must never change address (e.g. a mutex-guarded handle
///        registered by pointer with an OS callback).
class NonMovable {
protected:
    NonMovable() = default;
    ~NonMovable() = default;

public:
    NonMovable(const NonMovable&) = delete;
    NonMovable& operator=(const NonMovable&) = delete;
    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

} // namespace rmg::core
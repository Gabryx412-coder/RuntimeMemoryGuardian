// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/export.hpp
//
// Symbol visibility macros for building/consuming RuntimeMemoryGuardian as a
// shared library (RMG_BUILD_SHARED=ON). When built as a static library
// (the default), RMG_API expands to nothing and this header has no effect
// beyond documenting intent.
//
// Usage: annotate every publicly-exported class/function with RMG_API,
// e.g. `class RMG_API RuntimeMemoryGuardian { ... };`
// ==============================================================================

#pragma once

#if defined(RMG_SHARED_LIBRARY)
    #if defined(RMG_PLATFORM_WINDOWS)
        #if defined(RMG_BUILDING_LIBRARY)
            #define RMG_API __declspec(dllexport)
        #else
            #define RMG_API __declspec(dllimport)
        #endif
    #else
        #define RMG_API __attribute__((visibility("default")))
    #endif
#else
    #define RMG_API
#endif
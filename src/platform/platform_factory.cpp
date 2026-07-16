// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/platform_factory.cpp
//
// Implements rmg::platform::createPlatformTraits() by dispatching, at
// compile time, to the appropriate per-OS factory function
// (createWindowsPlatformTraits / createLinuxPlatformTraits). Exactly one of
// the two branches is compiled into any given build, per RMG_PLATFORM_*
// defined by cmake/RMGPlatform.cmake.
// ==============================================================================

#include <rmg/platform/platform_factory.hpp>

namespace rmg::platform {

#if defined(RMG_PLATFORM_WINDOWS)

std::unique_ptr<IPlatformTraits> createWindowsPlatformTraits();

std::unique_ptr<IPlatformTraits> createPlatformTraits() {
    return createWindowsPlatformTraits();
}

#elif defined(RMG_PLATFORM_LINUX)

std::unique_ptr<IPlatformTraits> createLinuxPlatformTraits();

std::unique_ptr<IPlatformTraits> createPlatformTraits() {
    return createLinuxPlatformTraits();
}

#else
#error "Runtime Memory Guardian: unsupported platform (platform_factory.cpp)"
#endif

} // namespace rmg::platform
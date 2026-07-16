# ==============================================================================
# Runtime Memory Guardian - Root CMake Configuration
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

# ------------------------------------------------------------------------------
# Project version is centralized here and propagated to cmake/RMGVersion.cmake.in
# ------------------------------------------------------------------------------
set(RMG_VERSION_MAJOR 0)
set(RMG_VERSION_MINOR 1)
set(RMG_VERSION_PATCH 0)
set(RMG_VERSION "${RMG_VERSION_MAJOR}.${RMG_VERSION_MINOR}.${RMG_VERSION_PATCH}")

project(RuntimeMemoryGuardian
    VERSION ${RMG_VERSION}
    DESCRIPTION "Defensive runtime memory integrity and hook detection library"
    HOMEPAGE_URL "https://github.com/yourorg/RuntimeMemoryGuardian"
    LANGUAGES CXX
)

# ------------------------------------------------------------------------------
# Global settings
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure out-of-source builds only.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "In-source builds are not allowed. Please create a separate 'build' "
        "directory and run CMake from there.")
endif()

# Export compile_commands.json for tooling (clang-tidy, IDEs, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if no build type was specified (single-config generators only).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# ------------------------------------------------------------------------------
# User-facing build options
# ------------------------------------------------------------------------------
option(RMG_BUILD_SHARED       "Build RuntimeMemoryGuardian as a shared library" OFF)
option(RMG_BUILD_TESTS        "Build unit and integration tests" ON)
option(RMG_BUILD_EXAMPLES     "Build example applications" ON)
option(RMG_BUILD_BENCHMARKS   "Build benchmark suite" OFF)
option(RMG_BUILD_TOOLS        "Build command-line tools" ON)
option(RMG_ENABLE_SANITIZERS  "Enable AddressSanitizer/UndefinedBehaviorSanitizer" OFF)
option(RMG_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(RMG_ENABLE_DOXYGEN     "Enable Doxygen documentation target" OFF)

# ------------------------------------------------------------------------------
# Include custom CMake modules
# ------------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(RMGPlatform)
include(RMGCompilerWarnings)
include(RMGSanitizers)

# ------------------------------------------------------------------------------
# Generate version.hpp from template
# ------------------------------------------------------------------------------
configure_file(
    "${CMAKE_SOURCE_DIR}/include/rmg/version.hpp.in"
    "${CMAKE_BINARY_DIR}/generated/rmg/version.hpp"
    @ONLY
)

# ------------------------------------------------------------------------------
# Subdirectories
# ------------------------------------------------------------------------------
add_subdirectory(src)

if(RMG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(RMG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(RMG_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(RMG_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# ------------------------------------------------------------------------------
# Installation / export (allows downstream `find_package(RuntimeMemoryGuardian)`)
# ------------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/include/rmg"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.hpp"
)

install(
    FILES "${CMAKE_BINARY_DIR}/generated/rmg/version.hpp"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rmg"
)

install(
    TARGETS rmg
    EXPORT RuntimeMemoryGuardianTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(
    EXPORT RuntimeMemoryGuardianTargets
    FILE RuntimeMemoryGuardianTargets.cmake
    NAMESPACE RMG::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/RuntimeMemoryGuardian"
)

configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/RuntimeMemoryGuardianConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/RuntimeMemoryGuardianConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/RuntimeMemoryGuardian"
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/RuntimeMemoryGuardianConfigVersion.cmake"
    VERSION ${RMG_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        "${CMAKE_BINARY_DIR}/RuntimeMemoryGuardianConfig.cmake"
        "${CMAKE_BINARY_DIR}/RuntimeMemoryGuardianConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/RuntimeMemoryGuardian"
)

# ------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------
message(STATUS "")
message(STATUS "Runtime Memory Guardian v${RMG_VERSION}")
message(STATUS "  Build type          : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared library       : ${RMG_BUILD_SHARED}")
message(STATUS "  Build tests          : ${RMG_BUILD_TESTS}")
message(STATUS "  Build examples       : ${RMG_BUILD_EXAMPLES}")
message(STATUS "  Build benchmarks     : ${RMG_BUILD_BENCHMARKS}")
message(STATUS "  Build tools          : ${RMG_BUILD_TOOLS}")
message(STATUS "  Sanitizers enabled   : ${RMG_ENABLE_SANITIZERS}")
message(STATUS "")
# ==============================================================================
# RMGPlatform.cmake
#
# Detects the target operating system and exposes:
#   - RMG_PLATFORM_WINDOWS / RMG_PLATFORM_LINUX  (CMake variables)
#   - Corresponding preprocessor definitions applied via rmg_apply_platform_defs()
# ==============================================================================

if(WIN32)
    set(RMG_PLATFORM_WINDOWS TRUE)
    set(RMG_PLATFORM_LINUX FALSE)
    set(RMG_PLATFORM_NAME "Windows")
elseif(UNIX AND NOT APPLE)
    set(RMG_PLATFORM_WINDOWS FALSE)
    set(RMG_PLATFORM_LINUX TRUE)
    set(RMG_PLATFORM_NAME "Linux")
else()
    message(FATAL_ERROR
        "Runtime Memory Guardian currently supports Windows and Linux only. "
        "Detected an unsupported platform.")
endif()

# ------------------------------------------------------------------------------
# rmg_apply_platform_defs
#
# Applies the correct RMG_PLATFORM_* compile definition to the given target,
# so that C++ code can use #if defined(RMG_PLATFORM_WINDOWS) / RMG_PLATFORM_LINUX
# without depending on compiler-specific macros directly.
# ------------------------------------------------------------------------------
function(rmg_apply_platform_defs target)
    if(RMG_PLATFORM_WINDOWS)
        target_compile_definitions(${target} PUBLIC RMG_PLATFORM_WINDOWS)
        target_compile_definitions(${target} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
    elseif(RMG_PLATFORM_LINUX)
        target_compile_definitions(${target} PUBLIC RMG_PLATFORM_LINUX)
    endif()
endfunction()
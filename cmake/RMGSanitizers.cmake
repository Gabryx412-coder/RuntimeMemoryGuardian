# ==============================================================================
# RMGSanitizers.cmake
#
# Optionally enables AddressSanitizer + UndefinedBehaviorSanitizer for a given
# target. Intended primarily for Debug builds and CI (see RMG_ENABLE_SANITIZERS).
# Not supported under MSVC in this configuration (use /fsanitize=address manually
# if needed).
# ==============================================================================

function(rmg_enable_sanitizers target)
    if(NOT RMG_ENABLE_SANITIZERS)
        return()
    endif()

    if(NOT TARGET ${target})
        message(FATAL_ERROR "rmg_enable_sanitizers: '${target}' is not a valid target")
    endif()

    if(MSVC)
        message(WARNING
            "RMG_ENABLE_SANITIZERS was requested but automatic sanitizer "
            "configuration is not implemented for MSVC in this build script. "
            "Skipping for target '${target}'.")
        return()
    endif()

    set(SANITIZER_FLAGS
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )

    target_compile_options(${target} PRIVATE ${SANITIZER_FLAGS})
    target_link_options(${target} PRIVATE ${SANITIZER_FLAGS})
endfunction()
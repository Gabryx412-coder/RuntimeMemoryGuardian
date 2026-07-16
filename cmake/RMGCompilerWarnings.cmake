# ==============================================================================
# RMGCompilerWarnings.cmake
#
# Centralizes compiler warning flags across the whole project so that every
# target (library, tests, examples, benchmarks, tools) is held to the same
# standard.
# ==============================================================================

function(rmg_enable_warnings target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "rmg_enable_warnings: '${target}' is not a valid target")
    endif()

    set(MSVC_WARNINGS
        /W4
        /permissive-
        /w14242 /w14254 /w14263 /w14265 /w14287
        /w14296 /w14311 /w14545 /w14546 /w14547
        /w14549 /w14555 /w14619 /w14640 /w14826
        /w14905 /w14906 /w14928
    )

    set(CLANG_GCC_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )

    if(MSVC)
        set(WARNING_FLAGS ${MSVC_WARNINGS})
    else()
        set(WARNING_FLAGS ${CLANG_GCC_WARNINGS})
    endif()

    if(RMG_WARNINGS_AS_ERRORS)
        if(MSVC)
            list(APPEND WARNING_FLAGS /WX)
        else()
            list(APPEND WARNING_FLAGS -Werror)
        endif()
    endif()

    target_compile_options(${target} PRIVATE ${WARNING_FLAGS})
endfunction()
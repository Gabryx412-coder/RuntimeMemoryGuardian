# ==============================================================================
# Runtime Memory Guardian - Library Target Definition
# ==============================================================================

set(RMG_PUBLIC_HEADERS_DIR "${CMAKE_SOURCE_DIR}/include")

# ------------------------------------------------------------------------------
# Source file lists, grouped by module to mirror include/rmg/*.
# ------------------------------------------------------------------------------

set(RMG_CORE_SOURCES
    core/error.cpp
    core/logger.cpp
)

set(RMG_UTILS_SOURCES
    utils/sha256.cpp
    utils/crc32.cpp
    utils/byte_buffer.cpp
    utils/string_utils.cpp
)

set(RMG_PLATFORM_SOURCES
    platform/platform_factory.cpp
)

set(RMG_PLATFORM_WINDOWS_SOURCES
    platform/windows/win_process_handle.cpp
    platform/windows/win_memory_region_enumerator.cpp
    platform/windows/win_module_enumerator.cpp
    platform/windows/win_platform_traits.cpp
)

set(RMG_PLATFORM_LINUX_SOURCES
    platform/linux/linux_process_handle.cpp
    platform/linux/linux_memory_region_enumerator.cpp
    platform/linux/linux_module_enumerator.cpp
    platform/linux/linux_platform_traits.cpp
)

set(RMG_MEMORY_SOURCES
    memory/memory_region_enumerator.cpp
    memory/memory_snapshot.cpp
    memory/memory_scanner.cpp
    memory/memory_diff.cpp
)

set(RMG_INTEGRITY_SOURCES
    integrity/hash_provider.cpp
    integrity/integrity_baseline.cpp
    integrity/integrity_checker.cpp
)

set(RMG_HOOKS_SOURCES
    hooks/hook_types.cpp
    hooks/inline_hook_detector.cpp
    hooks/iat_hook_detector.cpp
    hooks/eat_hook_detector.cpp
    hooks/hook_detector.cpp
)

set(RMG_MODULES_SOURCES
    modules/module_enumerator.cpp
    modules/module_monitor.cpp
)

set(RMG_PROCESS_SOURCES
    process/monitor_config.cpp
    process/monitor_event.cpp
    process/process_monitor.cpp
)

set(RMG_API_SOURCES
    api/runtime_memory_guardian.cpp
)

set(RMG_SOURCES
    ${RMG_CORE_SOURCES}
    ${RMG_UTILS_SOURCES}
    ${RMG_PLATFORM_SOURCES}
    ${RMG_MEMORY_SOURCES}
    ${RMG_INTEGRITY_SOURCES}
    ${RMG_HOOKS_SOURCES}
    ${RMG_MODULES_SOURCES}
    ${RMG_PROCESS_SOURCES}
    ${RMG_API_SOURCES}
)

if(RMG_PLATFORM_WINDOWS)
    list(APPEND RMG_SOURCES ${RMG_PLATFORM_WINDOWS_SOURCES})
elseif(RMG_PLATFORM_LINUX)
    list(APPEND RMG_SOURCES ${RMG_PLATFORM_LINUX_SOURCES})
endif()

# ------------------------------------------------------------------------------
# Library target
# ------------------------------------------------------------------------------

if(RMG_BUILD_SHARED)
    add_library(rmg SHARED ${RMG_SOURCES})
else()
    add_library(rmg STATIC ${RMG_SOURCES})
endif()

add_library(RMG::rmg ALIAS rmg)

target_compile_features(rmg PUBLIC cxx_std_20)

target_include_directories(rmg
    PUBLIC
        "$<BUILD_INTERFACE:${RMG_PUBLIC_HEADERS_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

rmg_apply_platform_defs(rmg)
rmg_enable_warnings(rmg)
rmg_enable_sanitizers(rmg)

if(RMG_BUILD_SHARED)
    target_compile_definitions(rmg
        PUBLIC RMG_SHARED_LIBRARY
        PRIVATE RMG_BUILDING_LIBRARY
    )
    set_target_properties(rmg PROPERTIES
        VERSION ${RMG_VERSION}
        SOVERSION ${RMG_VERSION_MAJOR}
    )
endif()

# ------------------------------------------------------------------------------
# Platform-specific system library linkage
# ------------------------------------------------------------------------------

if(RMG_PLATFORM_WINDOWS)
    target_link_libraries(rmg PRIVATE psapi)
elseif(RMG_PLATFORM_LINUX)
    find_package(Threads REQUIRED)
    target_link_libraries(rmg PUBLIC Threads::Threads)
endif()

set_target_properties(rmg PROPERTIES
    OUTPUT_NAME "rmg"
    POSITION_INDEPENDENT_CODE ON
)
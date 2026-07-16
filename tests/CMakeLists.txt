# ==============================================================================
# Runtime Memory Guardian - Test Suite
# ==============================================================================

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)

# Prevent GoogleTest from overriding our own compiler/linker settings.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# ------------------------------------------------------------------------------
# rmg_tests: single test binary aggregating every unit + integration test.
# ------------------------------------------------------------------------------

add_executable(rmg_tests
    test_main.cpp

    unit/core/test_error.cpp
    unit/core/test_logger.cpp

    unit/utils/test_sha256.cpp
    unit/utils/test_crc32.cpp

    unit/memory/test_memory_scanner.cpp
    unit/memory/test_memory_diff.cpp

    unit/integrity/test_hash_provider.cpp
    unit/integrity/test_integrity_checker.cpp

    unit/hooks/test_hook_detector.cpp

    unit/modules/test_module_monitor.cpp

    integration/test_full_scan_cycle.cpp
    integration/test_process_monitor_integration.cpp
)

target_link_libraries(rmg_tests
    PRIVATE
        RMG::rmg
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(rmg_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

rmg_enable_warnings(rmg_tests)
rmg_enable_sanitizers(rmg_tests)

include(GoogleTest)
gtest_discover_tests(rmg_tests)
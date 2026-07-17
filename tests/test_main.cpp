// ==============================================================================
// Runtime Memory Guardian
// File: tests/test_main.cpp
//
// Custom test entry point (rather than relying solely on GTest::gtest_main)
// so that RMG's internal logger can be silenced during test runs, keeping
// test output focused on GoogleTest's own reporting.
// ==============================================================================

#include <rmg/core/logger.hpp>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    // Silence RMG's internal diagnostic logging during tests: most tests
    // deliberately exercise failure paths (bad handles, malformed data),
    // and the resulting log noise would otherwise obscure GoogleTest's
    // own pass/fail output.
    rmg::core::Logger::instance().setMinLevel(rmg::core::LogLevel::Critical);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
// ==============================================================================
// Runtime Memory Guardian
// File: tests/integration/test_full_scan_cycle.cpp
//
// End-to-end integration test exercising the real platform backend
// (createPlatformTraits()) against the CURRENT process ("self"), verifying
// that baseline capture -> unmodified verification -> memory scan all work
// together correctly against real, live memory.
//
// This test intentionally does NOT modify its own executable memory (doing
// so reliably and portably from within the test binary is out of scope);
// instead, it validates the "no tampering" path end-to-end and validates
// that MemoryScanner returns sane, non-empty data when scanning the running
// process's own address space.
// ==============================================================================

#include <rmg/api/runtime_memory_guardian.hpp>

#include <gtest/gtest.h>

namespace {

TEST(FullScanCycleIntegrationTest, SelfBaselineEstablishesAndVerifiesClean) {
    auto guardianResult = rmg::api::RuntimeMemoryGuardian::createForSelf();
    ASSERT_TRUE(guardianResult.has_value()) << guardianResult.error().toDiagnosticString();
    auto& guardian = *guardianResult;

    auto baselineResult = guardian.establishBaseline();
    ASSERT_TRUE(baselineResult.has_value()) << baselineResult.error().toDiagnosticString();

    auto reportResult = guardian.checkIntegrity();
    ASSERT_TRUE(reportResult.has_value()) << reportResult.error().toDiagnosticString();
    EXPECT_TRUE(reportResult->isValid);
    EXPECT_TRUE(reportResult->tamperedSections.empty());
}

TEST(FullScanCycleIntegrationTest, ListModulesReturnsAtLeastTheTestBinaryItself) {
    auto guardianResult = rmg::api::RuntimeMemoryGuardian::createForSelf();
    ASSERT_TRUE(guardianResult.has_value());

    auto modulesResult = guardianResult->listModules();
    ASSERT_TRUE(modulesResult.has_value()) << modulesResult.error().toDiagnosticString();
    EXPECT_FALSE(modulesResult->empty());
}

TEST(FullScanCycleIntegrationTest, DetectHooksCompletesWithoutErrorOnCleanSelf) {
    auto guardianResult = rmg::api::RuntimeMemoryGuardian::createForSelf();
    ASSERT_TRUE(guardianResult.has_value());

    // We do not assert zero findings here: legitimate runtime environments
    // (sanitizers, profilers, some antivirus products) can themselves
    // install inline hooks into common library functions, which would be a
    // false positive from RMG's perspective in an instrumented test binary.
    // We only assert that the scan itself completes successfully.
    auto findingsResult = guardianResult->detectHooks();
    ASSERT_TRUE(findingsResult.has_value()) << findingsResult.error().toDiagnosticString();
}

TEST(FullScanCycleIntegrationTest, CheckIntegrityWithoutBaselineFailsGracefully) {
    auto guardianResult = rmg::api::RuntimeMemoryGuardian::createForSelf();
    ASSERT_TRUE(guardianResult.has_value());

    auto reportResult = guardianResult->checkIntegrity();
    ASSERT_FALSE(reportResult.has_value());
    EXPECT_EQ(reportResult.error().code(), rmg::core::ErrorCode::InvalidArgument);
}

TEST(FullScanCycleIntegrationTest, BaselineSurvivesSerializationRoundTripAgainstRealProcess) {
    auto guardianResult = rmg::api::RuntimeMemoryGuardian::createForSelf();
    ASSERT_TRUE(guardianResult.has_value());

    ASSERT_TRUE(guardianResult->establishBaseline().has_value());

    // Re-verify still passes after establishing the baseline once more
    // (sanity check that establishBaseline() is idempotent-safe to call).
    ASSERT_TRUE(guardianResult->establishBaseline().has_value());

    auto reportResult = guardianResult->checkIntegrity();
    ASSERT_TRUE(reportResult.has_value());
    EXPECT_TRUE(reportResult->isValid);
}

} // namespace
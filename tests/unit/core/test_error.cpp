// ==============================================================================
// Runtime Memory Guardian
// File: tests/unit/core/test_error.cpp
// ==============================================================================

#include <rmg/core/error.hpp>

#include <gtest/gtest.h>

namespace {

using rmg::core::Error;
using rmg::core::ErrorCode;

TEST(ErrorTest, ToStringReturnsStableNonEmptyNames) {
    EXPECT_EQ(rmg::core::toString(ErrorCode::Unknown), "Unknown");
    EXPECT_EQ(rmg::core::toString(ErrorCode::AccessDenied), "AccessDenied");
    EXPECT_EQ(rmg::core::toString(ErrorCode::NotFound), "NotFound");
    EXPECT_EQ(rmg::core::toString(ErrorCode::RegionNotFound), "RegionNotFound");
    EXPECT_EQ(rmg::core::toString(ErrorCode::InvalidArgument), "InvalidArgument");
    EXPECT_EQ(rmg::core::toString(ErrorCode::PlatformError), "PlatformError");
    EXPECT_EQ(rmg::core::toString(ErrorCode::MemoryAccessFailure), "MemoryAccessFailure");
    EXPECT_EQ(rmg::core::toString(ErrorCode::NotSupported), "NotSupported");
    EXPECT_EQ(rmg::core::toString(ErrorCode::SerializationError), "SerializationError");
    EXPECT_EQ(rmg::core::toString(ErrorCode::InternalError), "InternalError");
}

TEST(ErrorTest, DefaultConstructedErrorIsUnknownWithEmptyContext) {
    Error error;
    EXPECT_EQ(error.code(), ErrorCode::Unknown);
    EXPECT_TRUE(error.context().empty());
}

TEST(ErrorTest, ConstructWithCodeOnly) {
    Error error(ErrorCode::AccessDenied);
    EXPECT_EQ(error.code(), ErrorCode::AccessDenied);
    EXPECT_TRUE(error.context().empty());
}

TEST(ErrorTest, ConstructWithCodeAndContext) {
    Error error(ErrorCode::NotFound, "process 1234 does not exist");
    EXPECT_EQ(error.code(), ErrorCode::NotFound);
    EXPECT_EQ(error.context(), "process 1234 does not exist");
}

TEST(ErrorTest, DiagnosticStringIncludesCodeAndContextWhenPresent) {
    Error error(ErrorCode::PlatformError, "OpenProcess failed");
    const std::string diagnostic = error.toDiagnosticString();
    EXPECT_NE(diagnostic.find("PlatformError"), std::string::npos);
    EXPECT_NE(diagnostic.find("OpenProcess failed"), std::string::npos);
}

TEST(ErrorTest, DiagnosticStringOmitsContextWhenEmpty) {
    Error error(ErrorCode::InternalError);
    const std::string diagnostic = error.toDiagnosticString();
    EXPECT_EQ(diagnostic, "InternalError");
}

TEST(ErrorTest, FailHelperProducesUnexpectedWithCorrectPayload) {
    rmg::core::Result<int> result = rmg::core::fail<int>(ErrorCode::InvalidArgument, "bad value");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(result.error().context(), "bad value");
}

TEST(ErrorTest, ResultCanHoldSuccessValue) {
    rmg::core::Result<int> result = 42;
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

} // namespace
// ==============================================================================
// Runtime Memory Guardian
// File: tests/unit/core/test_logger.cpp
// ==============================================================================

#include <gtest/gtest.h>

#include <rmg/core/event.hpp>
#include <rmg/core/logger.hpp>

namespace {

using rmg::core::ILogSink;
using rmg::core::LogLevel;
using rmg::core::Logger;

/// @brief Test double capturing every log record it receives, so
///        assertions can verify Logger's filtering and dispatch behavior
///        without depending on real console output.
class RecordingSink final : public ILogSink {
public:
    struct Record {
        LogLevel level;
        std::string message;
    };

    void write(LogLevel level, std::string_view message) override {
        records.push_back(Record{level, std::string(message)});
    }

    std::vector<Record> records;
};

/// @brief RAII helper that restores Logger's previous sink/level after each
///        test, so tests do not leak configuration into one another (Logger
///        is a process-wide singleton).
class LoggerTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        previousLevel_ = Logger::instance().minLevel();
    }

    void TearDown() override {
        Logger::instance().setMinLevel(previousLevel_);
        Logger::instance().setSink(std::make_unique<rmg::core::ConsoleLogSink>());
    }

private:
    LogLevel previousLevel_ = LogLevel::Info;
};

TEST_F(LoggerTestFixture, LogsAreForwardedToConfiguredSink) {
    auto sink = std::make_unique<RecordingSink>();
    RecordingSink* sinkPtr = sink.get();

    Logger::instance().setSink(std::move(sink));
    Logger::instance().setMinLevel(LogLevel::Trace);

    Logger::instance().log(LogLevel::Info, "hello world");

    ASSERT_EQ(sinkPtr->records.size(), 1U);
    EXPECT_EQ(sinkPtr->records[0].level, LogLevel::Info);
    EXPECT_EQ(sinkPtr->records[0].message, "hello world");
}

TEST_F(LoggerTestFixture, MessagesBelowMinLevelAreFiltered) {
    auto sink = std::make_unique<RecordingSink>();
    RecordingSink* sinkPtr = sink.get();

    Logger::instance().setSink(std::move(sink));
    Logger::instance().setMinLevel(LogLevel::Warning);

    Logger::instance().log(LogLevel::Debug, "should be filtered");
    Logger::instance().log(LogLevel::Info, "should also be filtered");
    Logger::instance().log(LogLevel::Error, "should pass through");

    ASSERT_EQ(sinkPtr->records.size(), 1U);
    EXPECT_EQ(sinkPtr->records[0].message, "should pass through");
}

TEST_F(LoggerTestFixture, NullSinkSilentlyDropsAllMessages) {
    Logger::instance().setSink(nullptr);
    Logger::instance().setMinLevel(LogLevel::Trace);

    // Should not crash or throw despite there being no sink.
    EXPECT_NO_THROW(Logger::instance().log(LogLevel::Critical, "into the void"));
}

TEST_F(LoggerTestFixture, LogLevelToStringIsStableAndNonEmpty) {
    EXPECT_EQ(rmg::core::toString(LogLevel::Trace), "TRACE");
    EXPECT_EQ(rmg::core::toString(LogLevel::Debug), "DEBUG");
    EXPECT_EQ(rmg::core::toString(LogLevel::Info), "INFO");
    EXPECT_EQ(rmg::core::toString(LogLevel::Warning), "WARN");
    EXPECT_EQ(rmg::core::toString(LogLevel::Error), "ERROR");
    EXPECT_EQ(rmg::core::toString(LogLevel::Critical), "CRITICAL");
}

TEST(SignalTest, EmitInvokesAllConnectedListenersInOrder) {
    rmg::core::Signal<int> signal;
    std::vector<int> received;

    signal.connect([&received](int value) { received.push_back(value * 1); });
    signal.connect([&received](int value) { received.push_back(value * 10); });

    signal.emit(5);

    ASSERT_EQ(received.size(), 2U);
    EXPECT_EQ(received[0], 5);
    EXPECT_EQ(received[1], 50);
}

TEST(SignalTest, DisconnectRemovesOnlyTheSpecifiedListener) {
    rmg::core::Signal<int> signal;
    std::vector<int> received;

    rmg::core::Connection first = signal.connect([&received](int) { received.push_back(1); });
    signal.connect([&received](int) { received.push_back(2); });

    signal.disconnect(first);
    signal.emit(0);

    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(received[0], 2);
}

TEST(SignalTest, ListenerCountReflectsConnectionsAndDisconnections) {
    rmg::core::Signal<> signal;
    EXPECT_EQ(signal.listenerCount(), 0U);

    rmg::core::Connection connection = signal.connect([]() {});
    EXPECT_EQ(signal.listenerCount(), 1U);

    signal.disconnect(connection);
    EXPECT_EQ(signal.listenerCount(), 0U);
}

} // namespace
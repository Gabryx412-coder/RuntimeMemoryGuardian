// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/core/logger.hpp
//
// Minimal, dependency-free internal logging facility. RMG is a library meant
// to be embedded in security-sensitive host applications, so logging is:
//   - Off by default at Info level in Release builds (host decides verbosity)
//   - Pluggable via ILogSink, so a host application can route RMG diagnostics
//     into its own logging infrastructure instead of stdout.
// ==============================================================================

#pragma once

#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace rmg::core {

/// @brief Severity levels for log messages, ordered from least to most severe.
enum class LogLevel : std::uint8_t {
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

/// @brief Returns the stable, human-readable name of @p level (e.g. "WARN").
[[nodiscard]] std::string_view toString(LogLevel level) noexcept;

/// @brief Interface implemented by anything that wants to receive log
///        records emitted by RMG (console, file, host application logger...).
class ILogSink {
public:
    virtual ~ILogSink() = default;

    /// @brief Invoked for every log record whose level is >= the Logger's
    ///        configured minimum level. Implementations must be thread-safe,
    ///        as RMG may log from multiple internal threads (e.g. the
    ///        ProcessMonitor's polling loop).
    virtual void write(LogLevel level, std::string_view message) = 0;
};

/// @brief Simple ILogSink implementation that writes to stderr.
///
/// Used as the default sink so that diagnostics are visible out of the box
/// during development, without requiring the host application to configure
/// anything.
class ConsoleLogSink final : public ILogSink {
public:
    void write(LogLevel level, std::string_view message) override;
};

/// @brief Process-wide logging facility for Runtime Memory Guardian.
///
/// Access is provided via Logger::instance(); the logger itself is a thin,
/// mutex-guarded dispatcher over a caller-supplied ILogSink. Host
/// applications that want to silence or redirect RMG's diagnostics should
/// call setSink()/setMinLevel() once during startup.
class Logger final {
public:
    /// @brief Returns the single, process-wide Logger instance.
    [[nodiscard]] static Logger& instance();

    /// @brief Replaces the active sink. Passing nullptr disables logging
    ///        entirely (all calls to log() become no-ops).
    void setSink(std::unique_ptr<ILogSink> sink);

    /// @brief Sets the minimum severity that will be forwarded to the sink.
    void setMinLevel(LogLevel level) noexcept;

    /// @brief Returns the currently configured minimum severity.
    [[nodiscard]] LogLevel minLevel() const noexcept;

    /// @brief Logs @p message at @p level if it meets the configured
    ///        minimum severity. Thread-safe.
    void log(LogLevel level, std::string_view message);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger() = default;

    mutable std::mutex mutex_;
    std::unique_ptr<ILogSink> sink_;
    LogLevel minLevel_ = LogLevel::Info;
};

} // namespace rmg::core

/// @brief Convenience macros to keep call sites concise while still allowing
///        the logging calls to be compiled out entirely if ever needed.
#define RMG_LOG_TRACE(msg) ::rmg::core::Logger::instance().log(::rmg::core::LogLevel::Trace, msg)
#define RMG_LOG_DEBUG(msg) ::rmg::core::Logger::instance().log(::rmg::core::LogLevel::Debug, msg)
#define RMG_LOG_INFO(msg) ::rmg::core::Logger::instance().log(::rmg::core::LogLevel::Info, msg)
#define RMG_LOG_WARNING(msg)                                                                       \
    ::rmg::core::Logger::instance().log(::rmg::core::LogLevel::Warning, msg)
#define RMG_LOG_ERROR(msg) ::rmg::core::Logger::instance().log(::rmg::core::LogLevel::Error, msg)
#define RMG_LOG_CRITICAL(msg)                                                                      \
    ::rmg::core::Logger::instance().log(::rmg::core::LogLevel::Critical, msg)
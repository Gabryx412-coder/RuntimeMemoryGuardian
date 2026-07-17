// ==============================================================================
// Runtime Memory Guardian
// File: src/core/logger.cpp
// ==============================================================================

#include <rmg/core/logger.hpp>

#include <cstdio>

namespace rmg::core {

std::string_view toString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Critical:
            return "CRITICAL";
    }
    return "UNKNOWN";
}

void ConsoleLogSink::write(LogLevel level, std::string_view message) {
    std::FILE* stream = (level >= LogLevel::Error) ? stderr : stdout;
    std::fprintf(stream, "[RMG][%s] %.*s\n", std::string(toString(level)).c_str(),
                 static_cast<int>(message.size()), message.data());
}

Logger::Logger() : sink_(std::make_unique<ConsoleLogSink>()) {}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setSink(std::unique_ptr<ILogSink> sink) {
    std::lock_guard lock(mutex_);
    sink_ = std::move(sink);
}

void Logger::setMinLevel(LogLevel level) noexcept {
    std::lock_guard lock(mutex_);
    minLevel_ = level;
}

LogLevel Logger::minLevel() const noexcept {
    std::lock_guard lock(mutex_);
    return minLevel_;
}

void Logger::log(LogLevel level, std::string_view message) {
    std::lock_guard lock(mutex_);
    if (!sink_ || level < minLevel_) {
        return;
    }
    sink_->write(level, message);
}

} // namespace rmg::core
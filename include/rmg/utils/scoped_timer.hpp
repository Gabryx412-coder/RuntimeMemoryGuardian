// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/utils/scoped_timer.hpp
//
// RAII stopwatch utility. Primarily used by the benchmarks/ suite and,
// optionally, for diagnostic timing inside performance-sensitive paths such
// as ProcessMonitor's polling loop when Trace-level logging is enabled.
// ==============================================================================

#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <utility>

namespace rmg::utils {

/// @brief Measures the wall-clock duration of the scope in which it lives
///        and reports it via a caller-supplied callback upon destruction.
///
/// Example:
/// @code
/// {
///     ScopedTimer timer("scan", [](std::string_view label, std::chrono::nanoseconds elapsed) {
///         RMG_LOG_DEBUG(std::string(label) + " took " + std::to_string(elapsed.count()) + "ns");
///     });
///     doExpensiveScan();
/// } // duration reported here
/// @endcode
class ScopedTimer {
public:
    using ReportCallback = std::function<void(std::string_view label, std::chrono::nanoseconds elapsed)>;

    explicit ScopedTimer(std::string label, ReportCallback callback)
        : label_(std::move(label)),
          callback_(std::move(callback)),
          start_(std::chrono::steady_clock::now()) {}

    ~ScopedTimer() {
        if (callback_) {
            const auto elapsed = std::chrono::steady_clock::now() - start_;
            callback_(label_, std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed));
        }
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;

    /// @brief Returns the elapsed time since construction without stopping
    ///        the timer or triggering the report callback.
    [[nodiscard]] std::chrono::nanoseconds elapsedSoFar() const noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - start_);
    }

private:
    std::string label_;
    ReportCallback callback_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace rmg::utils
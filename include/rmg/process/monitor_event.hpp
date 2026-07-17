// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/process/monitor_event.hpp
//
// The unified event type ProcessMonitor reports to its consumers,
// regardless of which underlying subsystem (IntegrityChecker, HookDetector,
// ModuleMonitor) produced the observation. Unifying under one event type
// keeps the ProcessMonitor::onEvent signal's signature simple and lets
// consumers filter/branch on MonitorEventType as needed.
// ==============================================================================

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <rmg/hooks/hook_types.hpp>
#include <rmg/integrity/integrity_checker.hpp>
#include <rmg/modules/module_info.hpp>

namespace rmg::process {

/// @brief Categorizes the kind of observation a MonitorEvent represents.
enum class MonitorEventType : std::uint8_t {
    /// One or more baseline code sections no longer match their recorded
    /// digest (see MonitorEvent::tamperedSection).
    IntegrityViolation,

    /// A suspected hook was found by HookDetector (see
    /// MonitorEvent::hookFinding).
    HookDetected,

    /// A module was loaded that was not present at the previous poll (see
    /// MonitorEvent::moduleInfo).
    ModuleLoaded,

    /// A module that was previously loaded is no longer present (see
    /// MonitorEvent::moduleInfo).
    ModuleUnloaded,
};

/// @brief Returns a short, stable, human-readable name for @p type.
[[nodiscard]] std::string_view toString(MonitorEventType type) noexcept;

/// @brief A single observation emitted by ProcessMonitor during a
///        monitoring cycle.
///
/// Only the field(s) relevant to `type` are populated; the others remain at
/// their default (empty/nullopt) state. This is modeled as one struct with
/// optional payloads rather than a variant/union to keep the public API
/// straightforward to consume from simple callback code, at the modest cost
/// of a few unused optional members per instance.
struct MonitorEvent {
    MonitorEventType type = MonitorEventType::IntegrityViolation;

    /// @brief Human-readable summary suitable for direct logging/display.
    std::string description;

    /// @brief Wall-clock time at which this event was generated.
    std::chrono::system_clock::time_point timestamp;

    /// @brief Populated when type == IntegrityViolation.
    std::optional<rmg::integrity::TamperedSection> tamperedSection;

    /// @brief Populated when type == HookDetected.
    std::optional<rmg::hooks::HookFinding> hookFinding;

    /// @brief Populated when type == ModuleLoaded or type == ModuleUnloaded.
    std::optional<rmg::modules::ModuleInfo> moduleInfo;
};

} // namespace rmg::process
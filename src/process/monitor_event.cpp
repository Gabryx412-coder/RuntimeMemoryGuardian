// ==============================================================================
// Runtime Memory Guardian
// File: src/process/monitor_event.cpp
// ==============================================================================

#include <rmg/process/monitor_event.hpp>

#include <string_view>

namespace rmg::process {

std::string_view toString(MonitorEventType type) noexcept {
    switch (type) {
        case MonitorEventType::IntegrityViolation:
            return "IntegrityViolation";
        case MonitorEventType::HookDetected:
            return "HookDetected";
        case MonitorEventType::ModuleLoaded:
            return "ModuleLoaded";
        case MonitorEventType::ModuleUnloaded:
            return "ModuleUnloaded";
    }
    return "Unknown";
}

} // namespace rmg::process
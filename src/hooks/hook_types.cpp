// ==============================================================================
// Runtime Memory Guardian
// File: src/hooks/hook_types.cpp
// ==============================================================================

#include <rmg/hooks/hook_types.hpp>

namespace rmg::hooks {

std::string_view toString(HookType type) noexcept {
    switch (type) {
        case HookType::InlinePatch:     return "InlinePatch";
        case HookType::IatRedirection:  return "IatRedirection";
        case HookType::EatRedirection:  return "EatRedirection";
        case HookType::Unknown:         return "Unknown";
    }
    return "Unrecognized";
}

} // namespace rmg::hooks
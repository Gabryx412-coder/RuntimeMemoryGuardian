// ==============================================================================
// Runtime Memory Guardian
// File: src/process/monitor_config.cpp
//
// MonitorConfig is a plain aggregate fully defined in its header (default
// member initializers only, no behavior). This translation unit exists to
// keep a consistent one-header/one-source pairing across the process/
// module and as the designated location for any future validation logic
// (e.g. clamping pollInterval to a sane minimum).
//
// TODO(#monitor-config-validation): Add MonitorConfig::validate() to reject
//                                    pathological configurations (e.g.
//                                    pollInterval == 0) once ProcessMonitor
//                                    gains a documented minimum polling
//                                    interval.
// ==============================================================================

#include <rmg/process/monitor_config.hpp>

namespace rmg::process {

// Intentionally empty: see file header comment above.

} // namespace rmg::process
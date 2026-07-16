// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/api/runtime_memory_guardian.hpp
//
// The primary, high-level entry point for consumers of Runtime Memory
// Guardian. This Facade wires together the platform backend, memory
// scanner, integrity checker, hook detector, module monitor, and process
// monitor so that typical use cases require only a handful of calls,
// without the caller needing to understand or manually assemble the
// library's internal layering.
//
// Typical usage (self-integrity monitoring):
// @code
// auto guardianResult = rmg::api::RuntimeMemoryGuardian::createForSelf();
// if (!guardianResult) { / handle error / }
// auto& guardian = *guardianResult;
//
// auto baselineResult = guardian.establishBaseline();
// if (!baselineResult) { / handle error / }
//
// guardian.monitor().onEvent.connect([](const rmg::process::MonitorEvent& e) {
//     // react to integrity violations, hooks, module changes...
// });
// guardian.monitor().start();
// @endcode
// ==============================================================================

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <rmg/core/error.hpp>
#include <rmg/export.hpp>
#include <rmg/hooks/hook_detector.hpp>
#include <rmg/integrity/hash_provider.hpp>
#include <rmg/integrity/integrity_baseline.hpp>
#include <rmg/integrity/integrity_checker.hpp>
#include <rmg/memory/memory_scanner.hpp>
#include <rmg/modules/module_enumerator.hpp>
#include <rmg/platform/native_types.hpp>
#include <rmg/platform/platform_traits.hpp>
#include <rmg/platform/process_handle.hpp>
#include <rmg/process/process_monitor.hpp>

namespace rmg::api {

/// @brief High-level, batteries-included facade over Runtime Memory
///        Guardian's defensive monitoring capabilities.
///
/// RuntimeMemoryGuardian owns the ProcessHandle, platform backend, and
/// every subsystem it wires together; consumers do not need to construct
/// or manage any of RMG's internal components directly. For advanced
/// scenarios requiring finer control (e.g. custom hash providers, direct
/// access to individual subsystems), consumers may use the components in
/// rmg::memory, rmg::integrity, rmg::hooks, rmg::modules, and rmg::process
/// directly instead of this facade.
class RMG_API RuntimeMemoryGuardian final {
public:
    /// @brief Creates a guardian monitoring the current process ("self").
    ///        This is the typical use case: an application embeds RMG to
    ///        watch its own code sections for tampering.
    ///
    /// @param hashProvider Hash algorithm to use for integrity baselines
    ///                     and verification. Defaults to SHA-256.
    [[nodiscard]] static rmg::core::Result<RuntimeMemoryGuardian>
    createForSelf(std::unique_ptr<rmg::integrity::IHashProvider> hashProvider = rmg::integrity::makeDefaultHashProvider());

    /// @brief Creates a guardian monitoring an external process identified
    ///        by @p processId. Requires sufficient OS privileges to open
    ///        the target process for memory reading (see
    ///        rmg::platform::ProcessHandle::open).
    [[nodiscard]] static rmg::core::Result<RuntimeMemoryGuardian>
    createForProcess(rmg::platform::NativeProcessId processId,
                     std::unique_ptr<rmg::integrity::IHashProvider> hashProvider = rmg::integrity::makeDefaultHashProvider());

    ~RuntimeMemoryGuardian();

    RuntimeMemoryGuardian(RuntimeMemoryGuardian&&) noexcept;
    RuntimeMemoryGuardian& operator=(RuntimeMemoryGuardian&&) noexcept;
    RuntimeMemoryGuardian(const RuntimeMemoryGuardian&) = delete;
    RuntimeMemoryGuardian& operator=(const RuntimeMemoryGuardian&) = delete;

    /// @brief Captures a fresh integrity baseline covering every currently
    ///        loaded module's executable code sections, and installs it
    ///        into this guardian's ProcessMonitor for subsequent
    ///        checkIntegrity()/monitor() use.
    [[nodiscard]] rmg::core::Result<void> establishBaseline();

    /// @brief Loads a previously serialized baseline (see
    ///        rmg::integrity::IntegrityBaseline::serialize, and
    ///        tools/baseline-generator) and installs it for subsequent use.
    [[nodiscard]] rmg::core::Result<void> loadBaseline(rmg::core::ByteView serializedBaseline);

    /// @brief Verifies the currently installed baseline against the
    ///        target's present memory state. Fails with InvalidArgument if
    ///        no baseline has been established/loaded yet.
    [[nodiscard]] rmg::core::Result<rmg::integrity::IntegrityReport> checkIntegrity() const;

    /// @brief Runs hook detection across every currently loaded module in
    ///        the target process.
    [[nodiscard]] rmg::core::Result<std::vector<rmg::hooks::HookFinding>> detectHooks() const;

    /// @brief Enumerates every module currently loaded in the target
    ///        process.
    [[nodiscard]] rmg::core::Result<std::vector<rmg::modules::ModuleInfo>> listModules() const;

    /// @brief Provides direct access to the underlying ProcessMonitor for
    ///        continuous monitoring, event subscription, and configuration.
    [[nodiscard]] rmg::process::ProcessMonitor& monitor() noexcept { return *processMonitor_; }
    [[nodiscard]] const rmg::process::ProcessMonitor& monitor() const noexcept { return *processMonitor_; }

private:
    RuntimeMemoryGuardian(rmg::platform::ProcessHandle handle,
                         std::unique_ptr<rmg::platform::IPlatformTraits> platformTraits,
                         std::unique_ptr<rmg::integrity::IHashProvider> hashProvider);

    rmg::platform::ProcessHandle handle_;
    std::unique_ptr<rmg::platform::IPlatformTraits> platformTraits_;
    std::unique_ptr<rmg::integrity::IHashProvider> hashProvider_;

    std::unique_ptr<rmg::memory::MemoryScanner> scanner_;
    std::unique_ptr<rmg::modules::ModuleEnumerator> moduleEnumerator_;
    std::unique_ptr<rmg::hooks::HookDetector> hookDetector_;
    std::unique_ptr<rmg::integrity::IntegrityChecker> integrityChecker_;
    std::unique_ptr<rmg::process::ProcessMonitor> processMonitor_;
};

} // namespace rmg::api
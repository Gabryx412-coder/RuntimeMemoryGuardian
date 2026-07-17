// ==============================================================================
// Runtime Memory Guardian
// File: src/api/runtime_memory_guardian.cpp
// ==============================================================================

#include <rmg/api/runtime_memory_guardian.hpp>
#include <rmg/platform/platform_factory.hpp>

namespace rmg::api {

RuntimeMemoryGuardian::RuntimeMemoryGuardian(
    rmg::platform::ProcessHandle handle,
    std::unique_ptr<rmg::platform::IPlatformTraits> platformTraits,
    std::unique_ptr<rmg::integrity::IHashProvider> hashProvider)
    : handle_(std::move(handle)), platformTraits_(std::move(platformTraits)),
      hashProvider_(std::move(hashProvider)) {
    scanner_ = std::make_unique<rmg::memory::MemoryScanner>(*platformTraits_);
    moduleEnumerator_ = std::make_unique<rmg::modules::ModuleEnumerator>(*platformTraits_);
    hookDetector_ = std::make_unique<rmg::hooks::HookDetector>(*scanner_);
    integrityChecker_ =
        std::make_unique<rmg::integrity::IntegrityChecker>(*scanner_, *hashProvider_);
    processMonitor_ =
        std::make_unique<rmg::process::ProcessMonitor>(handle_, *platformTraits_, *hashProvider_);
}

RuntimeMemoryGuardian::~RuntimeMemoryGuardian() = default;
RuntimeMemoryGuardian::RuntimeMemoryGuardian(RuntimeMemoryGuardian&&) noexcept = default;
RuntimeMemoryGuardian& RuntimeMemoryGuardian::operator=(RuntimeMemoryGuardian&&) noexcept = default;

rmg::core::Result<RuntimeMemoryGuardian>
RuntimeMemoryGuardian::createForSelf(std::unique_ptr<rmg::integrity::IHashProvider> hashProvider) {
    auto handle = rmg::platform::ProcessHandle::openSelf();
    if (!handle) {
        return std::unexpected(handle.error());
    }

    auto platformTraits = rmg::platform::createPlatformTraits();
    return RuntimeMemoryGuardian(std::move(*handle), std::move(platformTraits),
                                 std::move(hashProvider));
}

rmg::core::Result<RuntimeMemoryGuardian> RuntimeMemoryGuardian::createForProcess(
    rmg::platform::NativeProcessId processId,
    std::unique_ptr<rmg::integrity::IHashProvider> hashProvider) {
    auto handle = rmg::platform::ProcessHandle::open(processId);
    if (!handle) {
        return std::unexpected(handle.error());
    }

    auto platformTraits = rmg::platform::createPlatformTraits();
    return RuntimeMemoryGuardian(std::move(*handle), std::move(platformTraits),
                                 std::move(hashProvider));
}

rmg::core::Result<void> RuntimeMemoryGuardian::establishBaseline() {
    auto modules = moduleEnumerator_->enumerate(handle_);
    if (!modules) {
        return std::unexpected(modules.error());
    }

    std::vector<rmg::integrity::CodeSectionInfo> allSections;
    for (const rmg::modules::ModuleInfo& module : *modules) {
        allSections.insert(allSections.end(), module.sections.begin(), module.sections.end());
    }

    auto baseline =
        rmg::integrity::IntegrityBaseline::create(allSections, handle_, *scanner_, *hashProvider_);
    if (!baseline) {
        return std::unexpected(baseline.error());
    }

    processMonitor_->setBaseline(std::move(*baseline));
    return {};
}

rmg::core::Result<void>
RuntimeMemoryGuardian::loadBaseline(rmg::core::ByteView serializedBaseline) {
    auto baseline = rmg::integrity::IntegrityBaseline::deserialize(serializedBaseline);
    if (!baseline) {
        return std::unexpected(baseline.error());
    }

    processMonitor_->setBaseline(std::move(*baseline));
    return {};
}

rmg::core::Result<rmg::integrity::IntegrityReport> RuntimeMemoryGuardian::checkIntegrity() const {
    const auto& installedBaseline = processMonitor_->baseline();
    if (!installedBaseline.has_value()) {
        return rmg::core::fail<rmg::integrity::IntegrityReport>(
            rmg::core::ErrorCode::InvalidArgument,
            "no baseline installed; call establishBaseline() or loadBaseline() first");
    }

    return integrityChecker_->verify(handle_, *installedBaseline);
}

rmg::core::Result<std::vector<rmg::hooks::HookFinding>> RuntimeMemoryGuardian::detectHooks() const {
    auto modules = moduleEnumerator_->enumerate(handle_);
    if (!modules) {
        return std::unexpected(modules.error());
    }

    std::vector<rmg::hooks::HookFinding> allFindings;
    for (const rmg::modules::ModuleInfo& module : *modules) {
        auto findings = hookDetector_->scanAll(handle_, module, *modules);
        if (!findings) {
            return std::unexpected(findings.error());
        }
        allFindings.insert(allFindings.end(), std::make_move_iterator(findings->begin()),
                           std::make_move_iterator(findings->end()));
    }

    return allFindings;
}

rmg::core::Result<std::vector<rmg::modules::ModuleInfo>>
RuntimeMemoryGuardian::listModules() const {
    return moduleEnumerator_->enumerate(handle_);
}

} // namespace rmg::api
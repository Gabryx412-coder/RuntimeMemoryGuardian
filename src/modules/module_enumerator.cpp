// ==============================================================================
// Runtime Memory Guardian
// File: src/modules/module_enumerator.cpp
// ==============================================================================

#include <rmg/modules/module_enumerator.hpp>
#include <rmg/utils/string_utils.hpp>

namespace rmg::modules {

namespace {

/// @brief Derives coarse-grained executable code sections for @p module by
///        selecting every committed, executable memory region that falls
///        within the module's address range.
///
/// Regions are named "region_N" (N = ordinal index within the module)
/// since /proc/maps and VirtualQueryEx do not report PE/ELF section names
/// directly; a future image-format parser can replace these with real
/// section names (".text", ".rdata", etc.) without changing this
/// function's contract.
[[nodiscard]] std::vector<rmg::integrity::CodeSectionInfo>
deriveCodeSections(const rmg::platform::NativeModuleInfo& module,
                   std::span<const rmg::platform::MemoryRegion> allRegions) {
    std::vector<rmg::integrity::CodeSectionInfo> sections;
    std::size_t ordinal = 0;

    const rmg::core::Address moduleEnd =
        module.baseAddress + static_cast<rmg::core::Address>(module.size);

    for (const rmg::platform::MemoryRegion& region : allRegions) {
        if (!region.isCommitted || !region.isExecutable()) {
            continue;
        }
        if (region.baseAddress < module.baseAddress || region.baseAddress >= moduleEnd) {
            continue;
        }

        rmg::integrity::CodeSectionInfo section;
        section.name = "region_" + std::to_string(ordinal++);
        section.baseAddress = region.baseAddress;
        section.size = region.size;
        section.ownerModule = module.name;
        section.ownerModulePath = module.path;

        sections.push_back(std::move(section));
    }

    return sections;
}

} // namespace

rmg::core::Result<std::vector<ModuleInfo>>
ModuleEnumerator::enumerate(const rmg::platform::ProcessHandle& handle) const {
    auto nativeModules = platformTraits_->enumerateModules(handle);
    if (!nativeModules) {
        return std::unexpected(nativeModules.error());
    }

    auto allRegions = platformTraits_->enumerateRegions(handle);
    if (!allRegions) {
        return std::unexpected(allRegions.error());
    }

    std::vector<ModuleInfo> result;
    result.reserve(nativeModules->size());

    for (const rmg::platform::NativeModuleInfo& nativeModule : *nativeModules) {
        ModuleInfo info;
        info.name = nativeModule.name;
        info.path = nativeModule.path;
        info.baseAddress = nativeModule.baseAddress;
        info.size = nativeModule.size;
        info.sections = deriveCodeSections(nativeModule, *allRegions);

        result.push_back(std::move(info));
    }

    return result;
}

rmg::core::Result<ModuleInfo>
ModuleEnumerator::findByName(const rmg::platform::ProcessHandle& handle,
                             std::string_view moduleName) const {
    auto allModules = enumerate(handle);
    if (!allModules) {
        return std::unexpected(allModules.error());
    }

    for (ModuleInfo& module : *allModules) {
        if (rmg::utils::iequals(module.name, moduleName)) {
            return std::move(module);
        }
    }

    return rmg::core::fail<ModuleInfo>(rmg::core::ErrorCode::NotFound,
                                       "no loaded module named '" + std::string(moduleName) + "'");
}

} // namespace rmg::modules
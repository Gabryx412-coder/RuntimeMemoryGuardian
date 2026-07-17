// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/windows/win_platform_traits.cpp
//
// Concrete IPlatformTraits implementation for Windows. Delegates the actual
// OS interaction to the free functions in win_process_handle.hpp,
// win_memory_region_enumerator.cpp, and win_module_enumerator.cpp; this
// class exists purely to satisfy the IPlatformTraits interface and is what
// platform_factory.cpp instantiates on Windows builds.
// ==============================================================================

#include "win_process_handle.hpp"

#include <memory>
#include <psapi.h>
#include <windows.h>

namespace rmg::platform {

namespace {

/// @brief Windows implementation of IPlatformTraits.
class WindowsPlatformTraits final : public IPlatformTraits {
public:
    [[nodiscard]] rmg::core::Result<std::vector<MemoryRegion>>
    enumerateRegions(const ProcessHandle& handle) const override {
        return detail::enumerateRegionsWindows(handle);
    }

    [[nodiscard]] rmg::core::Result<std::vector<NativeModuleInfo>>
    enumerateModules(const ProcessHandle& handle) const override {
        return detail::enumerateModulesWindows(handle);
    }

    [[nodiscard]] rmg::core::Result<std::size_t>
    readMemory(const ProcessHandle& handle, rmg::core::Address address,
               rmg::core::MutableByteView destination) const override {
        return detail::readMemoryWindows(handle, address, destination);
    }

    [[nodiscard]] rmg::core::Result<rmg::core::MemoryProtection>
    queryProtection(const ProcessHandle& handle, rmg::core::Address address) const override {
        return detail::queryProtectionWindows(handle, address);
    }

    [[nodiscard]] std::size_t pageSize() const noexcept override {
        SYSTEM_INFO systemInfo{};
        ::GetSystemInfo(&systemInfo);
        return static_cast<std::size_t>(systemInfo.dwPageSize);
    }
};

} // namespace

/// @brief Factory function invoked by platform_factory.cpp on Windows
///        builds. Kept as a free function (rather than exposing
///        WindowsPlatformTraits itself) so the class can remain in an
///        anonymous namespace, private to this translation unit.
std::unique_ptr<IPlatformTraits> createWindowsPlatformTraits() {
    return std::make_unique<WindowsPlatformTraits>();
}

} // namespace rmg::platform
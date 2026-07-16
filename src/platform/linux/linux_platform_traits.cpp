// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/linux/linux_platform_traits.cpp
//
// Concrete IPlatformTraits implementation for Linux. Delegates all OS
// interaction to the free functions declared in linux_process_handle.hpp;
// this class exists purely to satisfy the IPlatformTraits interface and is
// what platform_factory.cpp instantiates on Linux builds.
// ==============================================================================

#include "linux_process_handle.hpp"

#include <unistd.h>

namespace rmg::platform {

namespace {

/// @brief Linux implementation of IPlatformTraits.
class LinuxPlatformTraits final : public IPlatformTraits {
public:
    [[nodiscard]] rmg::core::Result<std::vector<MemoryRegion>>
    enumerateRegions(const ProcessHandle& handle) const override {
        return detail::enumerateRegionsLinux(handle);
    }

    [[nodiscard]] rmg::core::Result<std::vector<NativeModuleInfo>>
    enumerateModules(const ProcessHandle& handle) const override {
        return detail::enumerateModulesLinux(handle);
    }

    [[nodiscard]] rmg::core::Result<std::size_t>
    readMemory(const ProcessHandle& handle,
               rmg::core::Address address,
               rmg::core::MutableByteView destination) const override {
        return detail::readMemoryLinux(handle, address, destination);
    }

    [[nodiscard]] rmg::core::Result<rmg::core::MemoryProtection>
    queryProtection(const ProcessHandle& handle, rmg::core::Address address) const override {
        return detail::queryProtectionLinux(handle, address);
    }

    [[nodiscard]] std::size_t pageSize() const noexcept override {
        const long result = ::sysconf(_SC_PAGESIZE);
        return (result > 0) ? static_cast<std::size_t>(result) : 4096U;
    }
};

} // namespace

/// @brief Factory function invoked by platform_factory.cpp on Linux builds.
///        Kept as a free function so the class itself can stay private to
///        this translation unit (anonymous namespace).
std::unique_ptr<IPlatformTraits> createLinuxPlatformTraits() {
    return std::make_unique<LinuxPlatformTraits>();
}

} // namespace rmg::platform
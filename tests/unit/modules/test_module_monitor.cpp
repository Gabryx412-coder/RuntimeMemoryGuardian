// ==============================================================================
// Runtime Memory Guardian
// File: tests/unit/modules/test_module_monitor.cpp
//
// Verifies ModuleMonitor's load/unload event emission against a
// controllable ModuleEnumerator backed by a mock platform, whose reported
// module list can be changed between poll() calls.
// ==============================================================================

#include <gtest/gtest.h>

#include <rmg/modules/module_monitor.hpp>

namespace {

using rmg::platform::NativeModuleInfo;
using rmg::platform::ProcessHandle;

/// @brief Platform backend whose module list is mutable between poll()
///        calls, simulating modules being loaded/unloaded over time.
class MutableModuleListPlatform final : public rmg::platform::IPlatformTraits {
public:
    std::vector<NativeModuleInfo> modules;

    [[nodiscard]] rmg::core::Result<std::vector<rmg::platform::MemoryRegion>>
    enumerateRegions(const ProcessHandle&) const override {
        return std::vector<rmg::platform::MemoryRegion>{};
    }

    [[nodiscard]] rmg::core::Result<std::vector<NativeModuleInfo>>
    enumerateModules(const ProcessHandle&) const override {
        return modules;
    }

    [[nodiscard]] rmg::core::Result<std::size_t>
    readMemory(const ProcessHandle&, rmg::core::Address, rmg::core::MutableByteView) const override {
        return rmg::core::fail<std::size_t>(rmg::core::ErrorCode::NotSupported, "not used in this test");
    }

    [[nodiscard]] rmg::core::Result<rmg::core::MemoryProtection>
    queryProtection(const ProcessHandle&, rmg::core::Address) const override {
        return rmg::core::fail<rmg::core::MemoryProtection>(rmg::core::ErrorCode::NotSupported, "not used");
    }

    [[nodiscard]] std::size_t pageSize() const noexcept override { return 4096; }
};

TEST(ModuleMonitorTest, FirstPollReportsAllModulesAsLoaded) {
    MutableModuleListPlatform platform;
    platform.modules = {
        NativeModuleInfo{"a.dll", "/path/a.dll", 0x1000, 0x100},
        NativeModuleInfo{"b.dll", "/path/b.dll", 0x2000, 0x100},
    };

    rmg::modules::ModuleEnumerator enumerator(platform);
    rmg::modules::ModuleMonitor monitor(enumerator);

    std::vector<std::string> loadedNames;
    monitor.onModuleLoaded.connect(
        [&loadedNames](const rmg::modules::ModuleInfo& info) { loadedNames.push_back(info.name); });

    auto handle = ProcessHandle::openSelf();
    auto result = monitor.poll(*handle);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(loadedNames.size(), 2U);
    EXPECT_EQ(monitor.currentModules().size(), 2U);
}

TEST(ModuleMonitorTest, SubsequentPollWithNoChangesEmitsNoEvents) {
    MutableModuleListPlatform platform;
    platform.modules = {NativeModuleInfo{"stable.dll", "/path/stable.dll", 0x1000, 0x100}};

    rmg::modules::ModuleEnumerator enumerator(platform);
    rmg::modules::ModuleMonitor monitor(enumerator);

    auto handle = ProcessHandle::openSelf();
    ASSERT_TRUE(monitor.poll(*handle).has_value());

    int loadedCount = 0;
    int unloadedCount = 0;
    monitor.onModuleLoaded.connect([&loadedCount](const rmg::modules::ModuleInfo&) { ++loadedCount; });
    monitor.onModuleUnloaded.connect([&unloadedCount](const rmg::modules::ModuleInfo&) { ++unloadedCount; });

    ASSERT_TRUE(monitor.poll(*handle).has_value());

    EXPECT_EQ(loadedCount, 0);
    EXPECT_EQ(unloadedCount, 0);
}

TEST(ModuleMonitorTest, NewlyLoadedModuleTriggersOnModuleLoaded) {
    MutableModuleListPlatform platform;
    platform.modules = {NativeModuleInfo{"initial.dll", "/path/initial.dll", 0x1000, 0x100}};

    rmg::modules::ModuleEnumerator enumerator(platform);
    rmg::modules::ModuleMonitor monitor(enumerator);

    auto handle = ProcessHandle::openSelf();
    ASSERT_TRUE(monitor.poll(*handle).has_value());

    std::vector<std::string> newlyLoaded;
    monitor.onModuleLoaded.connect(
        [&newlyLoaded](const rmg::modules::ModuleInfo& info) { newlyLoaded.push_back(info.name); });

    platform.modules.push_back(NativeModuleInfo{"injected.dll", "/path/injected.dll", 0x3000, 0x100});
    ASSERT_TRUE(monitor.poll(*handle).has_value());

    ASSERT_EQ(newlyLoaded.size(), 1U);
    EXPECT_EQ(newlyLoaded[0], "injected.dll");
}

TEST(ModuleMonitorTest, RemovedModuleTriggersOnModuleUnloaded) {
    MutableModuleListPlatform platform;
    platform.modules = {
        NativeModuleInfo{"persistent.dll", "/path/persistent.dll", 0x1000, 0x100},
        NativeModuleInfo{"transient.dll", "/path/transient.dll", 0x2000, 0x100},
    };

    rmg::modules::ModuleEnumerator enumerator(platform);
    rmg::modules::ModuleMonitor monitor(enumerator);

    auto handle = ProcessHandle::openSelf();
    ASSERT_TRUE(monitor.poll(*handle).has_value());

    std::vector<std::string> unloaded;
    monitor.onModuleUnloaded.connect(
        [&unloaded](const rmg::modules::ModuleInfo& info) { unloaded.push_back(info.name); });

    platform.modules.erase(platform.modules.begin() + 1); // Remove "transient.dll".
    ASSERT_TRUE(monitor.poll(*handle).has_value());

    ASSERT_EQ(unloaded.size(), 1U);
    EXPECT_EQ(unloaded[0], "transient.dll");
}

} // namespace
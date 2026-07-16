// ==============================================================================
// Runtime Memory Guardian
// File: benchmarks/bench_hook_detector.cpp
//
// Measures the cost of running the combined HookDetector::scanAll() pass
// (inline + IAT + EAT) against every module loaded in the current process,
// representing the realistic per-cycle cost a ProcessMonitor consumer
// should expect when hook detection is enabled.
// ==============================================================================

#include <benchmark/benchmark.h>

#include <rmg/hooks/hook_detector.hpp>
#include <rmg/memory/memory_scanner.hpp>
#include <rmg/modules/module_enumerator.hpp>
#include <rmg/platform/platform_factory.hpp>

namespace {

class HookDetectorBenchmark : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State&) override {
        platformTraits = rmg::platform::createPlatformTraits();
        auto handleResult = rmg::platform::ProcessHandle::openSelf();
        handle = std::make_unique<rmg::platform::ProcessHandle>(std::move(*handleResult));

        scanner = std::make_unique<rmg::memory::MemoryScanner>(*platformTraits);
        moduleEnumerator = std::make_unique<rmg::modules::ModuleEnumerator>(*platformTraits);

        auto modulesResult = moduleEnumerator->enumerate(*handle);
        if (modulesResult) {
            modules = std::move(*modulesResult);
        }
    }

    void TearDown(const benchmark::State&) override {
        modules.clear();
        moduleEnumerator.reset();
        scanner.reset();
        handle.reset();
        platformTraits.reset();
    }

    std::unique_ptr<rmg::platform::IPlatformTraits> platformTraits;
    std::unique_ptr<rmg::platform::ProcessHandle> handle;
    std::unique_ptr<rmg::memory::MemoryScanner> scanner;
    std::unique_ptr<rmg::modules::ModuleEnumerator> moduleEnumerator;
    std::vector<rmg::modules::ModuleInfo> modules;
};

} // namespace

BENCHMARK_F(HookDetectorBenchmark, ScanAllModulesForHooks)(benchmark::State& state) {
    rmg::hooks::HookDetector detector(*scanner);

    for (auto _ : state) {
        for (const auto& module : modules) {
            auto findings = detector.scanAll(*handle, module, modules);
            benchmark::DoNotOptimize(findings);
        }
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(modules.size()));
}

BENCHMARK_F(HookDetectorBenchmark, ModuleEnumerationCost)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = moduleEnumerator->enumerate(*handle);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_MAIN();
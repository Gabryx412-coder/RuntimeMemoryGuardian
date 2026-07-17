// ==============================================================================
// Runtime Memory Guardian
// File: benchmarks/bench_memory_scanner.cpp
//
// Measures the cost of scanning the current process' own memory via
// MemoryScanner, across different filter configurations. Useful for
// understanding the overhead a host application should expect when calling
// checkIntegrity()/detectHooks() or running ProcessMonitor at a given
// pollInterval.
// ==============================================================================

#include <rmg/memory/memory_scanner.hpp>
#include <rmg/platform/platform_factory.hpp>

#include <benchmark/benchmark.h>

namespace {

/// @brief Shared fixture constructing the platform backend and self-handle
///        once per benchmark run, since these are relatively expensive to
///        set up and are not what we're trying to measure.
class MemoryScannerBenchmark : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State&) override {
        platformTraits = rmg::platform::createPlatformTraits();
        auto handleResult = rmg::platform::ProcessHandle::openSelf();
        handle = std::make_unique<rmg::platform::ProcessHandle>(std::move(*handleResult));
    }

    void TearDown(const benchmark::State&) override {
        handle.reset();
        platformTraits.reset();
    }

    std::unique_ptr<rmg::platform::IPlatformTraits> platformTraits;
    std::unique_ptr<rmg::platform::ProcessHandle> handle;
};

} // namespace

BENCHMARK_F(MemoryScannerBenchmark, EnumerateAllRegions)(benchmark::State& state) {
    rmg::memory::MemoryRegionEnumerator enumerator(*platformTraits);

    for (auto _ : state) {
        auto result = enumerator.enumerate(*handle);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(MemoryScannerBenchmark, EnumerateExecutableRegionsOnly)(benchmark::State& state) {
    rmg::memory::MemoryRegionEnumerator enumerator(*platformTraits);

    for (auto _ : state) {
        auto result = enumerator.enumerateExecutable(*handle);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(MemoryScannerBenchmark, FullScanAllRegions)(benchmark::State& state) {
    rmg::memory::MemoryScanner scanner(*platformTraits);

    for (auto _ : state) {
        auto result = scanner.scan(*handle, {});
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(MemoryScannerBenchmark, FullScanExecutableOnly)(benchmark::State& state) {
    rmg::memory::MemoryScanner scanner(*platformTraits);
    rmg::memory::ScanFilter filter;
    filter.executableOnly = true;

    for (auto _ : state) {
        auto result = scanner.scan(*handle, filter);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(MemoryScannerBenchmark, SmallDirectRead256Bytes)(benchmark::State& state) {
    rmg::memory::MemoryScanner scanner(*platformTraits);

    // Read from a known-valid address: the address of this very function,
    // guaranteed to be within a committed, executable region of the test
    // binary itself.
    const auto address = reinterpret_cast<rmg::core::Address>(&BM_SmallDirectRead256Bytes);

    for (auto _ : state) {
        auto result = scanner.read(*handle, address, 256);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_MAIN();
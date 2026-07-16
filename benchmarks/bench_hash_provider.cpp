// ==============================================================================
// Runtime Memory Guardian
// File: benchmarks/bench_hash_provider.cpp
//
// Compares SHA-256 vs CRC32 throughput across a range of buffer sizes,
// quantifying the tradeoff documented in
// rmg/integrity/hash_provider.hpp between cryptographic strength (SHA-256,
// the recommended default) and raw speed (CRC32, for cheap heuristic
// change-detection only).
// ==============================================================================

#include <benchmark/benchmark.h>

#include <vector>

#include <rmg/integrity/hash_provider.hpp>

namespace {

[[nodiscard]] std::vector<std::byte> makeTestBuffer(std::size_t size) {
    std::vector<std::byte> buffer(size);
    for (std::size_t i = 0; i < size; ++i) {
        buffer[i] = static_cast<std::byte>(i % 256);
    }
    return buffer;
}

} // namespace

static void BM_Sha256Hash(benchmark::State& state) {
    const std::size_t size = static_cast<std::size_t>(state.range(0));
    const std::vector<std::byte> buffer = makeTestBuffer(size);
    rmg::integrity::Sha256HashProvider provider;

    for (auto _ : state) {
        auto digest = provider.hash(rmg::core::ByteView(buffer.data(), buffer.size()));
        benchmark::DoNotOptimize(digest);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(size));
}
BENCHMARK(BM_Sha256Hash)->Arg(64)->Arg(1024)->Arg(65536)->Arg(1048576);

static void BM_Crc32Hash(benchmark::State& state) {
    const std::size_t size = static_cast<std::size_t>(state.range(0));
    const std::vector<std::byte> buffer = makeTestBuffer(size);
    rmg::integrity::Crc32HashProvider provider;

    for (auto _ : state) {
        auto digest = provider.hash(rmg::core::ByteView(buffer.data(), buffer.size()));
        benchmark::DoNotOptimize(digest);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(size));
}
BENCHMARK(BM_Crc32Hash)->Arg(64)->Arg(1024)->Arg(65536)->Arg(1048576);

BENCHMARK_MAIN();
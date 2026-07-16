// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/integrity/integrity_baseline.hpp
//
// Represents a "known-good" reference state of one or more code sections,
// captured once (typically at application startup, or offline via
// tools/baseline-generator) and later used by IntegrityChecker to detect
// tampering. A baseline stores per-section digests rather than raw
// contents, keeping it compact and safe to persist to disk.
// ==============================================================================

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include <rmg/core/error.hpp>
#include <rmg/integrity/code_section_info.hpp>
#include <rmg/integrity/hash_provider.hpp>
#include <rmg/memory/memory_scanner.hpp>
#include <rmg/platform/process_handle.hpp>

namespace rmg::integrity {

/// @brief One recorded entry within a baseline: a code section paired with
///        its digest at capture time.
struct BaselineEntry {
    CodeSectionInfo section;
    Digest digest;
};

/// @brief An immutable, known-good reference snapshot of code section
///        digests.
class IntegrityBaseline final {
public:
    /// @brief Captures a new baseline by reading each section in
    ///        @p sections from @p handle and hashing its contents with
    ///        @p hashProvider.
    ///
    /// @return A populated baseline on success. If a section's memory
    ///         cannot be fully read, that section is omitted from the
    ///         baseline and the omission is logged; callers can inspect
    ///         entries().size() against sections.size() to detect this.
    ///         Fails outright only if every section fails to read.
    [[nodiscard]] static rmg::core::Result<IntegrityBaseline>
    create(std::span<const CodeSectionInfo> sections,
           const rmg::platform::ProcessHandle& handle,
           const rmg::memory::MemoryScanner& scanner,
           const IHashProvider& hashProvider);

    IntegrityBaseline(const IntegrityBaseline&) = default;
    IntegrityBaseline& operator=(const IntegrityBaseline&) = default;
    IntegrityBaseline(IntegrityBaseline&&) = default;
    IntegrityBaseline& operator=(IntegrityBaseline&&) = default;

    [[nodiscard]] const std::vector<BaselineEntry>& entries() const noexcept { return entries_; }

    [[nodiscard]] std::chrono::system_clock::time_point capturedAt() const noexcept { return capturedAt_; }

    [[nodiscard]] std::string_view hashAlgorithmName() const noexcept { return hashAlgorithmName_; }

    /// @brief Serializes this baseline to a compact binary representation
    ///        suitable for writing to disk (see tools/baseline-generator).
    ///
    /// Format (little-endian):
    ///   magic:    4 bytes  "RMGB"
    ///   version:  u32
    ///   algoNameLen: u32, algoName: bytes
    ///   entryCount: u32
    ///   per entry: baseAddress(u64), size(u64), digestLen(u32), digest(bytes),
    ///              nameLen(u32) + name, ownerModuleLen(u32) + ownerModule
    [[nodiscard]] std::vector<std::byte> serialize() const;

    /// @brief Reconstructs an IntegrityBaseline previously produced by
    ///        serialize().
    [[nodiscard]] static rmg::core::Result<IntegrityBaseline> deserialize(rmg::core::ByteView data);

private:
    IntegrityBaseline() = default;

    std::chrono::system_clock::time_point capturedAt_;
    std::string hashAlgorithmName_;
    std::vector<BaselineEntry> entries_;
};

} // namespace rmg::integrity
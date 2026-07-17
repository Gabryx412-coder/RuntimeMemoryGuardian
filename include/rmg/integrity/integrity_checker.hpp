// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/integrity/integrity_checker.hpp
//
// Compares the current in-memory contents of a target process' code
// sections against a previously captured IntegrityBaseline, reporting any
// section whose digest no longer matches — the primary defensive mechanism
// against runtime code tampering.
// ==============================================================================

#pragma once

#include <rmg/core/error.hpp>
#include <rmg/integrity/code_section_info.hpp>
#include <rmg/integrity/hash_provider.hpp>
#include <rmg/integrity/integrity_baseline.hpp>
#include <rmg/memory/memory_scanner.hpp>
#include <rmg/platform/process_handle.hpp>

#include <vector>

namespace rmg::integrity {

/// @brief Describes a single section whose current digest no longer
///        matches its recorded baseline value.
struct TamperedSection {
    CodeSectionInfo section;
    Digest expectedDigest;
    Digest actualDigest;
};

/// @brief The outcome of a single IntegrityChecker::verify() call.
struct IntegrityReport {
    /// @brief True if every baseline section's current digest matched.
    bool isValid = true;

    /// @brief Every section found to have changed since the baseline was
    ///        captured. Empty when isValid is true.
    std::vector<TamperedSection> tamperedSections;

    /// @brief Sections present in the baseline that could not be read at
    ///        verification time (e.g. the owning module was unloaded).
    /// These are reported separately from tamperedSections because "cannot
    /// read" and "content changed" warrant different operator responses.
    std::vector<CodeSectionInfo> unreadableSections;
};

/// @brief Verifies a target process' code sections against a known-good
///        IntegrityBaseline.
class IntegrityChecker final {
public:
    /// @param scanner       Used to re-read each baseline section's current
    ///                      contents. Must outlive this object.
    /// @param hashProvider  Must produce digests compatible with those
    ///                      stored in the baselines this checker will
    ///                      verify (i.e. the same algorithm used at
    ///                      baseline-creation time). Must outlive this
    ///                      object.
    IntegrityChecker(const rmg::memory::MemoryScanner& scanner,
                     const IHashProvider& hashProvider) noexcept
        : scanner_(&scanner), hashProvider_(&hashProvider) {}

    /// @brief Verifies @p baseline against the current memory state of the
    ///        process referenced by @p handle.
    [[nodiscard]] rmg::core::Result<IntegrityReport>
    verify(const rmg::platform::ProcessHandle& handle, const IntegrityBaseline& baseline) const;

private:
    const rmg::memory::MemoryScanner* scanner_;
    const IHashProvider* hashProvider_;
};

} // namespace rmg::integrity
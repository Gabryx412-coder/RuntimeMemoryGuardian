// ==============================================================================
// Runtime Memory Guardian
// File: src/integrity/integrity_checker.cpp
// ==============================================================================

#include <rmg/core/logger.hpp>
#include <rmg/integrity/integrity_checker.hpp>

namespace rmg::integrity {

rmg::core::Result<IntegrityReport>
IntegrityChecker::verify(const rmg::platform::ProcessHandle& handle,
                         const IntegrityBaseline& baseline) const {
    IntegrityReport report;

    if (baseline.hashAlgorithmName() != hashProvider_->algorithmName()) {
        return rmg::core::fail<IntegrityReport>(
            rmg::core::ErrorCode::InvalidArgument,
            "baseline was captured with algorithm '" + std::string(baseline.hashAlgorithmName()) +
                "' but checker is configured with '" + std::string(hashProvider_->algorithmName()) +
                "'");
    }

    for (const BaselineEntry& entry : baseline.entries()) {
        auto buffer = scanner_->read(handle, entry.section.baseAddress, entry.section.size);
        if (!buffer) {
            RMG_LOG_WARNING("IntegrityChecker::verify: cannot read section '" + entry.section.name +
                            "' of '" + entry.section.ownerModule +
                            "': " + buffer.error().toDiagnosticString());
            report.unreadableSections.push_back(entry.section);
            continue;
        }

        Digest currentDigest = hashProvider_->hash(buffer->view());

        if (currentDigest != entry.digest) {
            report.isValid = false;
            report.tamperedSections.push_back(
                TamperedSection{entry.section, entry.digest, std::move(currentDigest)});
        }
    }

    return report;
}

} // namespace rmg::integrity
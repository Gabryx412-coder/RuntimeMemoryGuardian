// ==============================================================================
// Runtime Memory Guardian
// File: src/integrity/integrity_baseline.cpp
// ==============================================================================

#include <rmg/core/logger.hpp>
#include <rmg/integrity/integrity_baseline.hpp>

#include <array>
#include <cstring>

namespace rmg::integrity {

namespace {

constexpr std::array<char, 4> MAGIC = {'R', 'M', 'G', 'B'};
constexpr std::uint32_t FORMAT_VERSION = 1;

void appendU32(std::vector<std::byte>& buffer, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        buffer.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFFU));
    }
}

void appendU64(std::vector<std::byte>& buffer, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFFU));
    }
}

void appendBytes(std::vector<std::byte>& buffer, rmg::core::ByteView bytes) {
    buffer.insert(buffer.end(), bytes.begin(), bytes.end());
}

void appendString(std::vector<std::byte>& buffer, std::string_view text) {
    appendU32(buffer, static_cast<std::uint32_t>(text.size()));
    appendBytes(buffer,
                rmg::core::ByteView(reinterpret_cast<const std::byte*>(text.data()), text.size()));
}

/// @brief Small cursor-based reader used by deserialize() for bounds-checked
///        parsing of the binary baseline format.
class ByteReader {
public:
    explicit ByteReader(rmg::core::ByteView data) : data_(data) {}

    [[nodiscard]] bool readU32(std::uint32_t& out) {
        if (offset_ + 4 > data_.size()) {
            return false;
        }
        out = 0;
        for (int i = 0; i < 4; ++i) {
            out |= static_cast<std::uint32_t>(
                       static_cast<std::uint8_t>(data_[offset_ + static_cast<std::size_t>(i)]))
                   << (8 * i);
        }
        offset_ += 4;
        return true;
    }

    [[nodiscard]] bool readU64(std::uint64_t& out) {
        if (offset_ + 8 > data_.size()) {
            return false;
        }
        out = 0;
        for (int i = 0; i < 8; ++i) {
            out |= static_cast<std::uint64_t>(
                       static_cast<std::uint8_t>(data_[offset_ + static_cast<std::size_t>(i)]))
                   << (8 * i);
        }
        offset_ += 8;
        return true;
    }

    [[nodiscard]] bool readBytes(std::size_t length, rmg::core::ByteView& out) {
        if (offset_ + length > data_.size()) {
            return false;
        }
        out = data_.subspan(offset_, length);
        offset_ += length;
        return true;
    }

    [[nodiscard]] bool readString(std::string& out) {
        std::uint32_t length = 0;
        if (!readU32(length)) {
            return false;
        }
        rmg::core::ByteView bytes;
        if (!readBytes(length, bytes)) {
            return false;
        }
        out.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        return true;
    }

private:
    rmg::core::ByteView data_;
    std::size_t offset_ = 0;
};

} // namespace

rmg::core::Result<IntegrityBaseline> IntegrityBaseline::create(
    std::span<const CodeSectionInfo> sections, const rmg::platform::ProcessHandle& handle,
    const rmg::memory::MemoryScanner& scanner, const IHashProvider& hashProvider) {
    IntegrityBaseline baseline;
    baseline.capturedAt_ = std::chrono::system_clock::now();
    baseline.hashAlgorithmName_ = std::string(hashProvider.algorithmName());
    baseline.entries_.reserve(sections.size());

    for (const CodeSectionInfo& section : sections) {
        auto buffer = scanner.read(handle, section.baseAddress, section.size);
        if (!buffer) {
            RMG_LOG_WARNING("IntegrityBaseline::create: failed to read section '" + section.name +
                            "' of module '" + section.ownerModule +
                            "': " + buffer.error().toDiagnosticString());
            continue;
        }

        BaselineEntry entry;
        entry.section = section;
        entry.digest = hashProvider.hash(buffer->view());
        baseline.entries_.push_back(std::move(entry));
    }

    if (baseline.entries_.empty() && !sections.empty()) {
        return rmg::core::fail<IntegrityBaseline>(rmg::core::ErrorCode::MemoryAccessFailure,
                                                  "failed to capture any of the " +
                                                      std::to_string(sections.size()) +
                                                      " requested sections");
    }

    return baseline;
}

std::vector<std::byte> IntegrityBaseline::serialize() const {
    std::vector<std::byte> buffer;

    appendBytes(buffer, rmg::core::ByteView(reinterpret_cast<const std::byte*>(MAGIC.data()),
                                            MAGIC.size()));
    appendU32(buffer, FORMAT_VERSION);
    appendString(buffer, hashAlgorithmName_);
    appendU32(buffer, static_cast<std::uint32_t>(entries_.size()));

    for (const BaselineEntry& entry : entries_) {
        appendU64(buffer, static_cast<std::uint64_t>(entry.section.baseAddress));
        appendU64(buffer, static_cast<std::uint64_t>(entry.section.size));
        appendU32(buffer, static_cast<std::uint32_t>(entry.digest.size()));
        appendBytes(buffer, rmg::core::ByteView(entry.digest.data(), entry.digest.size()));
        appendString(buffer, entry.section.name);
        appendString(buffer, entry.section.ownerModule);
        appendString(buffer, entry.section.ownerModulePath);
    }

    return buffer;
}

rmg::core::Result<IntegrityBaseline> IntegrityBaseline::deserialize(rmg::core::ByteView data) {
    ByteReader reader(data);

    rmg::core::ByteView magicBytes;
    if (!reader.readBytes(MAGIC.size(), magicBytes) ||
        std::memcmp(magicBytes.data(), MAGIC.data(), MAGIC.size()) != 0) {
        return rmg::core::fail<IntegrityBaseline>(rmg::core::ErrorCode::SerializationError,
                                                  "invalid baseline magic header");
    }

    std::uint32_t version = 0;
    if (!reader.readU32(version) || version != FORMAT_VERSION) {
        return rmg::core::fail<IntegrityBaseline>(rmg::core::ErrorCode::SerializationError,
                                                  "unsupported baseline format version: " +
                                                      std::to_string(version));
    }

    IntegrityBaseline baseline;
    baseline.capturedAt_ = std::chrono::system_clock::now();

    if (!reader.readString(baseline.hashAlgorithmName_)) {
        return rmg::core::fail<IntegrityBaseline>(rmg::core::ErrorCode::SerializationError,
                                                  "truncated baseline: algorithm name");
    }

    std::uint32_t entryCount = 0;
    if (!reader.readU32(entryCount)) {
        return rmg::core::fail<IntegrityBaseline>(rmg::core::ErrorCode::SerializationError,
                                                  "truncated baseline: entry count");
    }

    baseline.entries_.reserve(entryCount);

    for (std::uint32_t i = 0; i < entryCount; ++i) {
        BaselineEntry entry;
        std::uint64_t baseAddress = 0;
        std::uint64_t size = 0;
        std::uint32_t digestLength = 0;
        rmg::core::ByteView digestBytes;

        if (!reader.readU64(baseAddress) || !reader.readU64(size) ||
            !reader.readU32(digestLength) || !reader.readBytes(digestLength, digestBytes) ||
            !reader.readString(entry.section.name) ||
            !reader.readString(entry.section.ownerModule) ||
            !reader.readString(entry.section.ownerModulePath)) {
            return rmg::core::fail<IntegrityBaseline>(rmg::core::ErrorCode::SerializationError,
                                                      "truncated baseline: entry " +
                                                          std::to_string(i));
        }

        entry.section.baseAddress = static_cast<rmg::core::Address>(baseAddress);
        entry.section.size = static_cast<std::size_t>(size);
        entry.digest.assign(digestBytes.begin(), digestBytes.end());

        baseline.entries_.push_back(std::move(entry));
    }

    return baseline;
}

} // namespace rmg::integrity
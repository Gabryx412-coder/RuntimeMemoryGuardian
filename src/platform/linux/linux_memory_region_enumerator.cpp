// ==============================================================================
// Runtime Memory Guardian
// File: src/platform/linux/linux_memory_region_enumerator.cpp
//
// Implements detail::parseProcMaps, detail::readProcMapsFile,
// detail::enumerateRegionsLinux, and detail::queryProtectionLinux by reading
// and parsing the textual /proc/[pid]/maps format:
//
//   <start>-<end> <perms> <offset> <dev> <inode> [pathname]
//
// Example line:
//   7f2a1c000000-7f2a1c021000 r-xp 00000000 08:01 131074  /usr/lib/x86_64-linux-gnu/libc.so.6
// ==============================================================================

#include "linux_process_handle.hpp"

#include <fstream>
#include <sstream>

namespace rmg::platform::detail {

namespace {

[[nodiscard]] rmg::core::Address parseHexAddress(std::string_view text) {
    rmg::core::Address value = 0;
    auto result = std::from_chars(text.data(), text.data() + text.size(), value, 16);
    (void)result; // Malformed input yields value == 0; treated as best-effort parsing.
    return value;
}

[[nodiscard]] std::string extractFileName(const std::string& fullPath) {
    const std::size_t lastSlash = fullPath.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return fullPath;
    }
    return fullPath.substr(lastSlash + 1);
}

} // namespace

std::vector<MemoryRegion> parseProcMaps(std::string_view mapsContent) {
    std::vector<MemoryRegion> regions;

    std::size_t lineStart = 0;
    while (lineStart < mapsContent.size()) {
        const std::size_t lineEnd = mapsContent.find('\n', lineStart);
        const std::string_view line = mapsContent.substr(
            lineStart, (lineEnd == std::string_view::npos ? mapsContent.size() : lineEnd) - lineStart);
        lineStart = (lineEnd == std::string_view::npos) ? mapsContent.size() : lineEnd + 1;

        if (line.empty()) {
            continue;
        }

        // Split "<start>-<end>" address range.
        const std::size_t dashPos = line.find('-');
        const std::size_t firstSpace = line.find(' ');
        if (dashPos == std::string_view::npos || firstSpace == std::string_view::npos || dashPos >= firstSpace) {
            continue; // Malformed line; skip defensively.
        }

        const std::string_view startText = line.substr(0, dashPos);
        const std::string_view endText = line.substr(dashPos + 1, firstSpace - dashPos - 1);

        const std::size_t permsStart = firstSpace + 1;
        const std::size_t secondSpace = line.find(' ', permsStart);
        if (secondSpace == std::string_view::npos) {
            continue;
        }
        const std::string_view perms = line.substr(permsStart, secondSpace - permsStart);

        // The pathname (if present) is the last whitespace-separated field.
        // /proc/[pid]/maps has 5 fixed fields before the optional pathname:
        // range, perms, offset, dev, inode.
        std::string pathname;
        const std::size_t pathCandidateStart = line.find_first_not_of(' ', line.find_last_of(' '));
        if (pathCandidateStart != std::string_view::npos) {
            std::string_view candidate = line.substr(pathCandidateStart);
            // A pathname always starts with '/' (file-backed) or '[' (special
            // mapping like [heap], [stack], [vdso]). Purely numeric trailing
            // fields (the inode) are not a pathname.
            if (!candidate.empty() && (candidate.front() == '/' || candidate.front() == '[')) {
                pathname = std::string(candidate);
            }
        }

        MemoryRegion region;
        region.baseAddress = parseHexAddress(startText);
        const rmg::core::Address endAddress = parseHexAddress(endText);
        region.size = (endAddress > region.baseAddress)
                          ? static_cast<std::size_t>(endAddress - region.baseAddress)
                          : 0;
        region.protection = parseProtectionString(perms);
        region.isCommitted = true; // /proc/maps only lists actually-mapped regions.

        if (!pathname.empty() && pathname.front() == '/') {
            region.modulePath = pathname;
            region.moduleName = extractFileName(pathname);
        }

        regions.push_back(std::move(region));
    }

    return regions;
}

rmg::core::Result<std::string> readProcMapsFile(const ProcessHandle& handle) {
    const std::string mapsPath = "/proc/" + std::to_string(handle.processId()) + "/maps";

    std::ifstream file(mapsPath, std::ios::in);
    if (!file.is_open()) {
        return rmg::core::fail<std::string>(
            rmg::core::ErrorCode::PlatformError,
            "failed to open " + mapsPath);
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

rmg::core::Result<std::vector<MemoryRegion>>
enumerateRegionsLinux(const ProcessHandle& handle) {
    auto mapsContent = readProcMapsFile(handle);
    if (!mapsContent) {
        return std::unexpected(mapsContent.error());
    }
    return parseProcMaps(*mapsContent);
}

rmg::core::Result<rmg::core::MemoryProtection>
queryProtectionLinux(const ProcessHandle& handle, rmg::core::Address address) {
    auto regions = enumerateRegionsLinux(handle);
    if (!regions) {
        return std::unexpected(regions.error());
    }

    for (const MemoryRegion& region : *regions) {
        if (region.contains(address)) {
            return region.protection;
        }
    }

    return rmg::core::fail<rmg::core::MemoryProtection>(
        rmg::core::ErrorCode::RegionNotFound,
        "no mapped region contains address " + std::to_string(address));
}

} // namespace rmg::platform::detail
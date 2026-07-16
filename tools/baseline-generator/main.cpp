// ==============================================================================
// Runtime Memory Guardian - rmg-baseline-generator
//
// Standalone tool for capturing an IntegrityBaseline from a running process
// and persisting it to disk, so it can later be shipped alongside an
// application and loaded at startup (via
// RuntimeMemoryGuardian::loadBaseline()) instead of re-establishing trust
// from the potentially-already-running, potentially-already-compromised
// process at every launch.
//
// Usage:
//   rmg-baseline-generator --self --output baseline.rmgbaseline
//   rmg-baseline-generator --pid 1234 --output baseline.rmgbaseline
//
// Exit codes:
//   0  Baseline successfully captured and written
//   1  Usage error or operational failure
// ==============================================================================

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>

#include <rmg/rmg.hpp>

namespace {

struct CliOptions {
    std::optional<rmg::platform::NativeProcessId> pid;
    bool targetSelf = false;
    std::optional<std::string> outputPath;
};

void printUsage() {
    std::printf(
        "rmg-baseline-generator - Capture an integrity baseline to disk\n\n"
        "Usage:\n"
        "  rmg-baseline-generator --self --output <path>\n"
        "  rmg-baseline-generator --pid <pid> --output <path>\n");
}

[[nodiscard]] std::optional<CliOptions> parseArgs(int argc, char** argv) {
    CliOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--self") {
            options.targetSelf = true;
        } else if (arg == "--pid") {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Error: --pid requires a value\n");
                return std::nullopt;
            }
            options.pid = static_cast<rmg::platform::NativeProcessId>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--output") {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Error: --output requires a path\n");
                return std::nullopt;
            }
            options.outputPath = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            std::exit(0);
        } else {
            std::fprintf(stderr, "Error: unrecognized argument '%s'\n", arg.c_str());
            return std::nullopt;
        }
    }

    return options;
}

[[nodiscard]] bool writeFileBytes(const std::string& path, const std::vector<std::byte>& data) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return file.good();
}

} // namespace

int main(int argc, char** argv) {
    auto parsedOptions = parseArgs(argc, argv);
    if (!parsedOptions.has_value()) {
        printUsage();
        return 1;
    }
    const CliOptions& options = *parsedOptions;

    if (!options.targetSelf && !options.pid.has_value()) {
        std::fprintf(stderr, "Error: must specify a target with --self or --pid <pid>\n\n");
        printUsage();
        return 1;
    }

    if (!options.outputPath.has_value()) {
        std::fprintf(stderr, "Error: --output <path> is required\n\n");
        printUsage();
        return 1;
    }

    std::printf("rmg-baseline-generator v%s\n", std::string(rmg::VERSION_STRING).c_str());

    auto platformTraits = rmg::platform::createPlatformTraits();

    auto handleResult = options.targetSelf
        ? rmg::platform::ProcessHandle::openSelf()
        : rmg::platform::ProcessHandle::open(*options.pid);

    if (!handleResult) {
        std::fprintf(stderr, "Error: failed to open target process: %s\n",
                     handleResult.error().toDiagnosticString().c_str());
        return 1;
    }

    rmg::memory::MemoryScanner scanner(*platformTraits);
    rmg::modules::ModuleEnumerator moduleEnumerator(*platformTraits);
    rmg::integrity::Sha256HashProvider hashProvider;

    std::printf("Enumerating loaded modules...\n");
    auto modulesResult = moduleEnumerator.enumerate(*handleResult);
    if (!modulesResult) {
        std::fprintf(stderr, "Error: failed to enumerate modules: %s\n",
                     modulesResult.error().toDiagnosticString().c_str());
        return 1;
    }

    std::vector<rmg::integrity::CodeSectionInfo> allSections;
    for (const auto& module : *modulesResult) {
        allSections.insert(allSections.end(), module.sections.begin(), module.sections.end());
    }
    std::printf("Found %zu module(s), %zu code section(s) total.\n",
                modulesResult->size(), allSections.size());

    std::printf("Capturing baseline (hashing with %s)...\n", std::string(hashProvider.algorithmName()).c_str());
    auto baselineResult = rmg::integrity::IntegrityBaseline::create(
        allSections, *handleResult, scanner, hashProvider);
    if (!baselineResult) {
        std::fprintf(stderr, "Error: failed to capture baseline: %s\n",
                     baselineResult.error().toDiagnosticString().c_str());
        return 1;
    }
    std::printf("Baseline captured: %zu section(s) hashed successfully.\n",
                baselineResult->entries().size());

    std::vector<std::byte> serialized = baselineResult->serialize();
    if (!writeFileBytes(*options.outputPath, serialized)) {
        std::fprintf(stderr, "Error: failed to write baseline to '%s'\n", options.outputPath->c_str());
        return 1;
    }

    std::printf("Baseline written to '%s' (%zu bytes).\n", options.outputPath->c_str(), serialized.size());
    return 0;
}
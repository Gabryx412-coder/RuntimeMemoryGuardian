// ==============================================================================
// Runtime Memory Guardian - rmg-cli
//
// Command-line front-end for Runtime Memory Guardian, allowing ad-hoc
// integrity checks, hook scans, and module listing against a running
// process without writing any code.
//
// Usage:
//   rmg-cli --self                      Target the current process (rmg-cli itself)
//   rmg-cli --pid <pid>                 Target an external process by id
//   rmg-cli --list-modules              List loaded modules
//   rmg-cli --check-integrity           Establish a baseline and verify it immediately
//   rmg-cli --detect-hooks              Scan all loaded modules for suspected hooks
//   rmg-cli --load-baseline <path>      Load a serialized baseline and verify against it
//   rmg-cli --version                   Print version information
//   rmg-cli --help                      Show this help text
//
// Exit codes:
//   0  Success / no anomalies found
//   1  Usage error or operational failure (I/O, process access, etc.)
//   2  Anomalies found (integrity violation and/or hooks detected)
// ==============================================================================

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <rmg/rmg.hpp>

namespace {

struct CliOptions {
    std::optional<rmg::platform::NativeProcessId> pid;
    bool targetSelf = false;
    bool listModules = false;
    bool checkIntegrity = false;
    bool detectHooks = false;
    std::optional<std::string> loadBaselinePath;
    bool showVersion = false;
    bool showHelp = false;
};

void printUsage() {
    std::printf(
        "rmg-cli - Runtime Memory Guardian command-line tool\n\n"
        "Usage:\n"
        "  rmg-cli --self                    Target the current process\n"
        "  rmg-cli --pid <pid>               Target an external process by id\n"
        "  rmg-cli --list-modules            List loaded modules\n"
        "  rmg-cli --check-integrity         Establish and immediately verify a baseline\n"
        "  rmg-cli --detect-hooks            Scan all loaded modules for suspected hooks\n"
        "  rmg-cli --load-baseline <path>    Load a serialized baseline and verify it\n"
        "  rmg-cli --version                 Print version information\n"
        "  rmg-cli --help                    Show this help text\n");
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
        } else if (arg == "--list-modules") {
            options.listModules = true;
        } else if (arg == "--check-integrity") {
            options.checkIntegrity = true;
        } else if (arg == "--detect-hooks") {
            options.detectHooks = true;
        } else if (arg == "--load-baseline") {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Error: --load-baseline requires a path\n");
                return std::nullopt;
            }
            options.loadBaselinePath = argv[++i];
        } else if (arg == "--version") {
            options.showVersion = true;
        } else if (arg == "--help" || arg == "-h") {
            options.showHelp = true;
        } else {
            std::fprintf(stderr, "Error: unrecognized argument '%s'\n", arg.c_str());
            return std::nullopt;
        }
    }

    return options;
}

[[nodiscard]] rmg::core::Result<std::vector<std::byte>> readFileBytes(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return rmg::core::fail<std::vector<std::byte>>(
            rmg::core::ErrorCode::NotFound, "cannot open file: " + path);
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return rmg::core::fail<std::vector<std::byte>>(
            rmg::core::ErrorCode::SerializationError, "failed to read file: " + path);
    }

    return buffer;
}

void printModules(const std::vector<rmg::modules::ModuleInfo>& modules) {
    std::printf("Loaded modules (%zu):\n", modules.size());
    for (const auto& module : modules) {
        std::printf("  0x%016zx  size=%-10zu sections=%-3zu %s\n",
                    static_cast<std::size_t>(module.baseAddress),
                    module.size,
                    module.sections.size(),
                    module.name.c_str());
    }
}

void printIntegrityReport(const rmg::integrity::IntegrityReport& report) {
    if (report.isValid) {
        std::printf("[OK] No integrity violations detected.\n");
    } else {
        std::printf("[ALERT] %zu tampered section(s) found:\n", report.tamperedSections.size());
        for (const auto& tampered : report.tamperedSections) {
            std::printf("  - %s!%s at 0x%zx\n",
                        tampered.section.ownerModule.c_str(),
                        tampered.section.name.c_str(),
                        static_cast<std::size_t>(tampered.section.baseAddress));
        }
    }

    if (!report.unreadableSections.empty()) {
        std::printf("[WARN] %zu section(s) were unreadable during verification.\n",
                     report.unreadableSections.size());
    }
}

void printHookFindings(const std::vector<rmg::hooks::HookFinding>& findings) {
    if (findings.empty()) {
        std::printf("[OK] No suspected hooks found.\n");
        return;
    }

    std::printf("[ALERT] %zu suspected hook(s) found:\n", findings.size());
    for (const auto& finding : findings) {
        std::printf("  - [%s] %s (module: %s)\n",
                    std::string(rmg::hooks::toString(finding.type)).c_str(),
                    finding.description.c_str(),
                    finding.moduleName.c_str());
    }
}

} // namespace

int main(int argc, char** argv) {
    auto parsedOptions = parseArgs(argc, argv);
    if (!parsedOptions.has_value()) {
        printUsage();
        return 1;
    }
    const CliOptions& options = *parsedOptions;

    if (options.showHelp || argc == 1) {
        printUsage();
        return 0;
    }

    if (options.showVersion) {
        std::printf("rmg-cli (Runtime Memory Guardian) v%s\n", std::string(rmg::VERSION_STRING).c_str());
        return 0;
    }

    if (!options.targetSelf && !options.pid.has_value()) {
        std::fprintf(stderr, "Error: must specify a target with --self or --pid <pid>\n\n");
        printUsage();
        return 1;
    }

    auto guardianResult = options.targetSelf
        ? rmg::api::RuntimeMemoryGuardian::createForSelf()
        : rmg::api::RuntimeMemoryGuardian::createForProcess(*options.pid);

    if (!guardianResult) {
        std::fprintf(stderr, "Error: failed to attach to target process: %s\n",
                     guardianResult.error().toDiagnosticString().c_str());
        return 1;
    }
    auto& guardian = *guardianResult;

    bool anomalyFound = false;
    bool ranAnyOperation = false;

    if (options.listModules) {
        ranAnyOperation = true;
        auto modules = guardian.listModules();
        if (!modules) {
            std::fprintf(stderr, "Error: failed to list modules: %s\n",
                         modules.error().toDiagnosticString().c_str());
            return 1;
        }
        printModules(*modules);
    }

    if (options.loadBaselinePath.has_value()) {
        ranAnyOperation = true;
        auto fileBytes = readFileBytes(*options.loadBaselinePath);
        if (!fileBytes) {
            std::fprintf(stderr, "Error: %s\n", fileBytes.error().toDiagnosticString().c_str());
            return 1;
        }

        auto loadResult = guardian.loadBaseline(
            rmg::core::ByteView(fileBytes->data(), fileBytes->size()));
        if (!loadResult) {
            std::fprintf(stderr, "Error: failed to load baseline: %s\n",
                         loadResult.error().toDiagnosticString().c_str());
            return 1;
        }

        auto report = guardian.checkIntegrity();
        if (!report) {
            std::fprintf(stderr, "Error: failed to verify integrity: %s\n",
                         report.error().toDiagnosticString().c_str());
            return 1;
        }
        printIntegrityReport(*report);
        anomalyFound = anomalyFound || !report->isValid;
    } else if (options.checkIntegrity) {
        ranAnyOperation = true;
        auto baselineResult = guardian.establishBaseline();
        if (!baselineResult) {
            std::fprintf(stderr, "Error: failed to establish baseline: %s\n",
                         baselineResult.error().toDiagnosticString().c_str());
            return 1;
        }

        auto report = guardian.checkIntegrity();
        if (!report) {
            std::fprintf(stderr, "Error: failed to verify integrity: %s\n",
                         report.error().toDiagnosticString().c_str());
            return 1;
        }
        printIntegrityReport(*report);
        anomalyFound = anomalyFound || !report->isValid;
    }

    if (options.detectHooks) {
        ranAnyOperation = true;
        auto findings = guardian.detectHooks();
        if (!findings) {
            std::fprintf(stderr, "Error: hook detection failed: %s\n",
                         findings.error().toDiagnosticString().c_str());
            return 1;
        }
        printHookFindings(*findings);
        anomalyFound = anomalyFound || !findings->empty();
    }

    if (!ranAnyOperation) {
        std::fprintf(stderr, "Error: no operation specified (use --list-modules, "
                             "--check-integrity, --detect-hooks, or --load-baseline)\n\n");
        printUsage();
        return 1;
    }

    return anomalyFound ? 2 : 0;
}
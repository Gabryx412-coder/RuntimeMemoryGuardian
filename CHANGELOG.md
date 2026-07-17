# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Executable image-format parsers (`PeImageReader`, `ElfImageReader`) to complete
  IAT/EAT hook detection with real import/export table walking (see
  `docs/architecture/HOOK_DETECTOR.md`).
- Named PE/ELF section reporting in `ModuleEnumerator` (currently reports
  coarse-grained `region_N` sections derived from memory region boundaries).

## [0.1.0] - Unreleased

### Added
- Initial public architecture: `core`, `platform`, `memory`, `integrity`,
  `hooks`, `modules`, `process`, and `api` layers.
- Cross-platform process handle, memory region enumeration, and memory
  reading abstraction (`rmg::platform`) for Windows and Linux.
- Memory scanning and snapshot/diff primitives (`rmg::memory`).
- SHA-256 (from scratch, FIPS 180-4 compliant) and CRC32 hash providers,
  integrity baseline capture with binary serialization, and integrity
  verification (`rmg::integrity`).
- Inline hook detection via documented redirection-opcode pattern matching;
  IAT/EAT validation logic with a clearly scoped extension point for a
  future image-format parser (`rmg::hooks`).
- Module enumeration and load/unload change monitoring (`rmg::modules`).
- Continuous and on-demand process monitoring orchestration with a unified
  event model (`rmg::process`).
- High-level `RuntimeMemoryGuardian` facade (`rmg::api`).
- `rmg-cli` and `rmg-baseline-generator` command-line tools.
- Full unit and integration test suite (GoogleTest).
- Benchmark suite (Google Benchmark) for memory scanning, hashing, and hook
  detection.
- Five progressive usage examples.
- CI pipelines for Linux (GCC/Clang) and Windows (MSVC), plus a dedicated
  code-quality workflow (clang-format, clang-tidy, sanitizers).
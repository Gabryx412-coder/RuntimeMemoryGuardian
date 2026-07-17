# Integrity Checker Architecture

## Responsibility

`rmg::integrity` establishes trust in a set of code sections at one point in time (the *baseline*) and later verifies that trust still holds.

## Components

- **`IHashProvider`** — abstracts the digest algorithm. `Sha256HashProvider` (recommended, cryptographically strong) and `Crc32HashProvider` (fast, non-cryptographic heuristic) are provided; consumers can implement their own.
- **`IntegrityBaseline`** — captures digests for a list of `CodeSectionInfo` at a point in time, and can serialize/deserialize to a compact binary format (magic `"RMGB"`, versioned) for persistence across process restarts.
- **`IntegrityChecker`** — re-reads each baselined section's current contents and compares digests, producing an `IntegrityReport` distinguishing `tamperedSections` (digest mismatch) from `unreadableSections` (couldn't be read at verification time — e.g. the owning module was unloaded).

## Why Digests, Not Raw Snapshots?

Storing digests rather than raw bytes keeps baselines small enough to persist to disk and ship alongside an application (see `tools/baseline-generator`), while still detecting any change with cryptographic confidence (when using SHA-256).

## Algorithm Compatibility

`IntegrityChecker::verify()` rejects a baseline captured with a different algorithm than the checker is configured with (`ErrorCode::InvalidArgument`), rather than silently producing a meaningless comparison — mixing algorithms between baseline creation and verification is a configuration error, not a runtime condition to tolerate.

## Known Limitation

Code sections are currently derived by `ModuleEnumerator` from raw executable memory-region boundaries (`region_N` naming), not from parsed PE/ELF section tables. This means separately-linked `.text`/`.text$mn`/etc. sections that happen to be mapped contiguously are reported as one section. See `HOOK_DETECTOR.md` for the planned image-format parser that will also improve this.
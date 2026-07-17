# Memory Scanner Architecture

## Responsibility

`rmg::memory` is responsible for turning raw platform-level primitives (`IPlatformTraits::enumerateRegions`, `IPlatformTraits::readMemory`) into higher-level, ergonomic operations: filtered scans, immutable snapshots, and byte-level diffing.

## Components

- **`MemoryRegionEnumerator`** — thin wrapper listing regions, with an `enumerateExecutable()` convenience filter. Kept separate from `MemoryScanner` so components that only need region metadata (no content) — like `ModuleEnumerator`'s section derivation — don't pay for content capture.
- **`MemoryScanner`** — the main entry point. `scan()` combines enumeration + filtering (`ScanFilter`: module name, executable-only, W^X exclusion) + content capture into one `MemorySnapshot`. `read()` performs a single direct read when the caller already knows the address range.
- **`MemorySnapshot`** — an immutable, movable capture of one or more regions' contents at a point in time. Deliberately move-only (not copyable) since it may hold large buffers.
- **`MemoryDiff`** — a stateless comparator between two snapshots, producing merged, contiguous `ByteRangeChange` entries rather than one entry per differing byte.

## Design Notes

- Partial reads (e.g. a region straddling an unmapped guard page) are treated as **partial successes**, not errors: the captured buffer is shrunk to the bytes actually read. Callers relying on exact sizes should check `SnapshotRegion::contents.size()` against the requested region size.
- `MemoryDiff::compare()` also detects region-set mismatches (a region present in one snapshot but not the other) via `DiffResult::regionSetIdentical`, separately from byte-level content changes — a module being unloaded entirely is a different signal than one of its bytes changing.
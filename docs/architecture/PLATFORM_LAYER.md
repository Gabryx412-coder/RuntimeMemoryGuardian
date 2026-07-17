# Platform Layer Architecture

## Responsibility

`rmg::platform` is the single Dependency Inversion seam of the entire library. It defines `IPlatformTraits`, an abstract interface covering every OS interaction RMG needs, and hides all concrete OS API usage behind it.

## The Rule

**No header outside `include/rmg/platform/` and `src/platform/` includes an OS-native header** (`<windows.h>`, `<unistd.h>`, `<sys/*.h>`, etc.). This is enforced by convention and code review, not by tooling, but it is the load-bearing architectural constraint that makes the rest of the library platform-agnostic.

## Windows Backend (`src/platform/windows/`)

- Process handles: `OpenProcess`/`DuplicateHandle` (for self) + `CloseHandle`.
- Region enumeration: `VirtualQueryEx` walking the address space, resolving backing files via `GetMappedFileNameA`.
- Module enumeration: `EnumProcessModulesEx` + `GetModuleInformation` + `GetModuleFileNameExA`.
- Memory reads: `ReadProcessMemory`.
- Protection translation: Win32 `PAGE_*` constants ↔ `rmg::core::MemoryProtection`.

## Linux Backend (`src/platform/linux/`)

- Process handles: existence check via `kill(pid, 0)`, then `open("/proc/[pid]/mem", O_RDONLY)`, kept open for the handle's lifetime.
- Region enumeration: parsing `/proc/[pid]/maps` (exposed separately as `parseProcMaps()` for unit testing against synthetic input).
- Module enumeration: collapsing contiguous file-backed `/proc/[pid]/maps` entries sharing a path into one logical module.
- Memory reads: `pread()` on the cached `/proc/[pid]/mem` file descriptor.
- Protection translation: `rwxp`-style permission strings ↔ `rmg::core::MemoryProtection`.

## Adding a New Platform

See [CONTRIBUTING.md](../../CONTRIBUTING.md#adding-support-for-a-new-platform) for the step-by-step process.
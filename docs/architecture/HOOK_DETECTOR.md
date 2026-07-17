# Hook Detector Architecture

## Responsibility

`rmg::hooks` detects signs of common, documented hooking techniques: inline patches, Import Address Table (IAT) redirection, and Export Address Table (EAT) redirection. All detection is **read-only**; RMG never removes, patches, or "cleans" a detected hook.

## Components

- **`InlineHookDetector`** — ✅ fully implemented. Inspects the first bytes of each code section for known redirection opcodes:
  - `E9 rel32` (near relative `JMP`)
  - `FF 25 disp32` (near absolute `JMP` via `[rip+disp32]`, x64)
  - `68 imm32 C3` (`PUSH imm32; RET` trampoline idiom)

  A finding is only raised when the decoded target lands **outside** the owning module's image — a jump within the same module is common in legitimate compiler-generated thunks.

- **`IatHookDetector`** / **`EatHookDetector`** — validation logic implemented and unit-tested (`resolvesIntoKnownModule` / `resolvesWithinOwnModule`); the executable-image-format table walk (parsing the PE import/export directory, or the ELF PLT/GOT and dynamic symbol table) is a documented, explicit extension point (see `TODO(#iat-image-parser)` / `TODO(#eat-image-parser)` in the corresponding `.cpp` files). Both currently return no findings pending this parser.

- **`HookDetector`** — facade aggregating all three strategies via `scanAll()`, tolerating partial failure of the IAT/EAT passes without aborting the inline scan.

## Planned Work: Image Format Parser

A future `PeImageReader` (Windows) / `ElfImageReader` (Linux) component will:

1. Parse a module's import directory / PLT-GOT to yield resolved IAT slot addresses per imported symbol.
2. Parse a module's export directory / dynamic symbol table to yield resolved addresses per exported symbol, distinguishing genuine forwarded exports from unexpected redirection.
3. Additionally enable `ModuleEnumerator` to report real section names (`.text`, `.rdata`, ...) instead of synthesized `region_N` placeholders.

This is intentionally scoped as future work rather than a rushed, unreliable implementation: correctly parsing two different executable formats (PE and ELF) is a substantial undertaking that deserves its own dedicated design and test suite.

## False Positives

Legitimate runtime environments (sanitizers, profilers, some security/monitoring tools) can install their own inline hooks into common library functions. `InlineHookDetector` findings should be treated as **signals to investigate**, not incontrovertible proof of compromise — this is inherent to heuristic, pattern-based defensive scanning.
// ==============================================================================
// Runtime Memory Guardian
// File: src/utils/byte_buffer.cpp
//
// ByteBuffer is fully defined in the header (small, value-type-like class
// with no out-of-line logic required). This translation unit exists so that
// the class has a single, stable definition point for tooling (e.g. to host
// explicit template instantiations or out-of-line helpers) should the class
// grow non-trivial logic in the future, and so the build system has a
// consistent one-header/one-source pairing across the utils/ module.
//
// TODO(#byte-buffer-io): Add ByteBuffer::toHexDump()/fromHexString() helpers
//                         here once the tools/rmg-cli diagnostic output format
//                         is finalized (Parte 17).
// ==============================================================================

#include <rmg/utils/byte_buffer.hpp>

namespace rmg::utils {

// Intentionally empty: see file header comment above.

} // namespace rmg::utils
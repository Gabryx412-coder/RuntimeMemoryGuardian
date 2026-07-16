// ==============================================================================
// Runtime Memory Guardian
// File: include/rmg/utils/byte_buffer.hpp
//
// Owning, RAII byte container used wherever the library needs to hold onto
// memory contents it has read (snapshots, scan results) rather than merely
// viewing them. Wraps std::vector<std::byte> with a small, purpose-specific
// interface tailored to how RMG uses buffers, plus convenient conversions
// to/from ByteView.
// ==============================================================================

#pragma once

#include <cstddef>
#include <vector>

#include <rmg/core/types.hpp>

namespace rmg::utils {

/// @brief An owning buffer of raw bytes.
///
/// ByteBuffer is copyable and movable with standard std::vector semantics.
/// It exists primarily to give a named, intention-revealing type to
/// "a chunk of memory RMG owns", as opposed to the non-owning
/// rmg::core::ByteView used for parameters and read-only access.
class ByteBuffer {
public:
    ByteBuffer() = default;

    /// @brief Constructs a buffer of @p size bytes, value-initialized to zero.
    explicit ByteBuffer(std::size_t size) : data_(size, std::byte{0}) {}

    /// @brief Constructs a buffer by copying the contents of @p view.
    explicit ByteBuffer(rmg::core::ByteView view) : data_(view.begin(), view.end()) {}

    /// @brief Returns a read-only view over the buffer's contents.
    [[nodiscard]] rmg::core::ByteView view() const noexcept {
        return rmg::core::ByteView(data_.data(), data_.size());
    }

    /// @brief Returns a mutable view over the buffer's contents.
    [[nodiscard]] rmg::core::MutableByteView mutableView() noexcept {
        return rmg::core::MutableByteView(data_.data(), data_.size());
    }

    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }
    [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

    [[nodiscard]] std::byte* data() noexcept { return data_.data(); }
    [[nodiscard]] const std::byte* data() const noexcept { return data_.data(); }

    /// @brief Resizes the buffer, preserving existing content and
    ///        zero-filling any newly added bytes (std::vector::resize semantics).
    void resize(std::size_t newSize) { data_.resize(newSize, std::byte{0}); }

    /// @brief Releases all storage, leaving the buffer empty.
    void clear() noexcept { data_.clear(); }

    [[nodiscard]] std::byte& operator[](std::size_t index) noexcept { return data_[index]; }
    [[nodiscard]] const std::byte& operator[](std::size_t index) const noexcept { return data_[index]; }

    [[nodiscard]] bool operator==(const ByteBuffer& other) const = default;

private:
    std::vector<std::byte> data_;
};

} // namespace rmg::utils
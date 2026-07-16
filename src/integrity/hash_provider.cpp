// ==============================================================================
// Runtime Memory Guardian
// File: src/integrity/hash_provider.cpp
// ==============================================================================

#include <rmg/integrity/hash_provider.hpp>

#include <rmg/utils/crc32.hpp>
#include <rmg/utils/sha256.hpp>

namespace rmg::integrity {

Digest Sha256HashProvider::hash(rmg::core::ByteView data) const {
    const std::array<std::byte, rmg::utils::Sha256::DIGEST_SIZE> digestArray =
        rmg::utils::Sha256::hash(data);
    return Digest(digestArray.begin(), digestArray.end());
}

std::string_view Sha256HashProvider::algorithmName() const noexcept {
    return "SHA-256";
}

Digest Crc32HashProvider::hash(rmg::core::ByteView data) const {
    const std::uint32_t crc = rmg::utils::crc32(data);
    Digest digest(sizeof(crc));
    digest[0] = static_cast<std::byte>((crc >> 24) & 0xFFU);
    digest[1] = static_cast<std::byte>((crc >> 16) & 0xFFU);
    digest[2] = static_cast<std::byte>((crc >> 8) & 0xFFU);
    digest[3] = static_cast<std::byte>(crc & 0xFFU);
    return digest;
}

std::string_view Crc32HashProvider::algorithmName() const noexcept {
    return "CRC32";
}

std::unique_ptr<IHashProvider> makeDefaultHashProvider() {
    return std::make_unique<Sha256HashProvider>();
}

} // namespace rmg::integrity
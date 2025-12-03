#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Compression
{

    static constexpr uint32_t RANS_M_BITS = 16;
    static constexpr uint32_t RANS_M = 1 << RANS_M_BITS; // Normalization threshold
    static constexpr uint32_t RANS_L_BITS = 8;
    static constexpr uint32_t RANS_L = 1 << RANS_L_BITS; // Sum of frequencies. Must be power-of-two
    static constexpr uint32_t RANS_ALPHABET_SIZE = 256;

    /// @brief Compress input data using rANS with a 256-byte alphabet and TOTFREQ = 256.
    /// Stream format:
    ///   0: 3 bytes uncompressed size
    ///   3: 1 byte rANS type marker "0x40"
    ///   4: 256 bytes symbol frequencies
    /// 260: 4 bytes initial rANS state
    /// 264: N compressed data
    auto encodeRANS(const std::vector<uint8_t> &data) -> std::vector<uint8_t>;

    /// @brief Decompress input data using rANS with a 256-byte alphabet and TOTFREQ = 256
    auto decodeRANS(const std::vector<uint8_t> &data) -> std::vector<uint8_t>;
}

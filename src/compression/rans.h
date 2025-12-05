#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Compression
{

#define RANS_M_BITS 14
    static constexpr uint32_t RANS_M = 1 << RANS_M_BITS; // Total sum of frequencies. Must be power-of-two
#define RANS_L_BITS 23
    static constexpr uint32_t RANS_L = 1 << RANS_L_BITS; // Lower bound of our normalization interval
    static constexpr uint32_t RANS_ALPHABET_SIZE = 256;

    static constexpr uint8_t RANS_HEADER_MIN_BITS = 6;                            // Bits attributed to minimum count
    static constexpr uint8_t RANS_HEADER_MODE_MASK = 3 << RANS_HEADER_MIN_BITS;   // Bits attributed to header mode
    static constexpr uint8_t RANS_HEADER_MODE_SINGLE = 0 << RANS_HEADER_MIN_BITS; // Flag value for single-symbol mode
    static constexpr uint8_t RANS_HEADER_MODE_256 = 1 << RANS_HEADER_MIN_BITS;    // Flag value for 256 count mode
    static constexpr uint8_t RANS_HEADER_MODE_RLE = 2 << RANS_HEADER_MIN_BITS;    // Flag value for RLE mode (not implemented)

    /// @brief Compress input data using rANS with a 256-byte alphabet and RANS_M = 256.
    /// Stream format:
    ///       0: 3 bytes uncompressed size
    ///       3: 1 byte rANS type marker "0x40"
    ///       4: 1 byte rANS header mode
    ///       5: C bytes symbol frequencies (256 or 512 bytes)
    ///     5+C: 1 byte initial rANS state size S
    ///   5+C+1: 0-4 byte initial rANS state
    /// 5+C+1+S: N compressed data
    auto encodeRANS(const std::vector<uint8_t> &data) -> std::vector<uint8_t>;

    /// @brief Decompress input data using rANS with a 256-byte alphabet and RANS_M = 256
    auto decodeRANS(const std::vector<uint8_t> &data) -> std::vector<uint8_t>;
}

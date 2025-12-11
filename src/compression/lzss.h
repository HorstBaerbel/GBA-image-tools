#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Compression
{

    constexpr uint8_t LZSS_TYPE_MARKER = 0x10;          // Used to detect LZ4 compression in data
    constexpr uint32_t LZSS_MIN_MATCH_LENGTH = 3;       // Match encoding takes 2 bytes, so our match must be longer
    constexpr uint32_t LZSS_MAX_MATCH_LENGTH = 18;      // We have max. 4 bits to encode match length [3,18]
    constexpr uint32_t LZSS_MAX_MATCH_DISTANCE = 0xFFF; // We have max. 12 bits to encode match distance

    /// @brief Compress input data using LZSS variant 10
    /// Compatible with : https://problemkaputt.de/gbatek.htm#biosdecompressionfunctions
    auto encodeLZ10(const std::vector<uint8_t> &data, bool vramCompatible = false) -> std::vector<uint8_t>;

    /// @brief Decompress input data using LZSS variant 10
    auto decodeLZ10(const std::vector<uint8_t> &data, bool vramCompatible = false) -> std::vector<uint8_t>;
}

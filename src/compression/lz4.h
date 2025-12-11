#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Compression
{

    constexpr uint8_t LZ4_TYPE_MARKER = 0x40;          // Used to detect LZ4 compression in data
    constexpr uint32_t LZ4_MAX_LITERAL_LENGTH = 15;    // We have max. 4 bits to encode literal length in the first token
    constexpr uint32_t LZ4_MIN_MATCH_LENGTH = 4;       // A match needs at least 3 bytes to encode, thus 4 is the minimum match length
    constexpr uint32_t LZ4_MAX_MATCH_LENGTH = 529;     // We want max. 4+8+8 bits to encode match length -> 529 = 15+255+255 + LZ4_MIN_MATCH_LENGTH
    constexpr uint32_t LZ4_MAX_MATCH_DISTANCE = 65535; // We have max. 16 bits to encode match distance
    constexpr uint8_t LZ4_LITERAL_LENGTH_MASK = 0xF0;
    constexpr uint8_t LZ4_MATCH_LENGTH_MASK = 0x0F;

    /// @brief Compress input data using LZ4 variant 40h
    /// @note This is probably not 100% stream compatible with regular LZ4
    auto encodeLZ4_40(const std::vector<uint8_t> &data, bool vramCompatible = false) -> std::vector<uint8_t>;

    /// @brief Decompress input data using LZ4 variant 40h
    auto decodeLZ4_40(const std::vector<uint8_t> &data, bool vramCompatible = false) -> std::vector<uint8_t>;
}

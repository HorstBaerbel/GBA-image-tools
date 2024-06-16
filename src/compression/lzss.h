#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Compression
{

    /// @brief Compress input data using LZSS variant 10
    /// Compatible with : https://problemkaputt.de/gbatek.htm#biosdecompressionfunctions
    auto encodeLZ10(const std::vector<uint8_t> &data, bool vramCompatible = false) -> std::vector<uint8_t>;

    /// @brief Decompress input data using LZSS variant 10
    auto decodeLZ10(const std::vector<uint8_t> &data, bool vramCompatible = false) -> std::vector<uint8_t>;
}

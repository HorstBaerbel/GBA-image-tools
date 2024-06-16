#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Compression
{

    /// @brief Find path to gbalzss executable
    /// @return Path if found, empty string if not found
    std::string findGbalzss();

    /// @brief Compress input data using lzss variant 10 or 11 and return the data
    std::vector<uint8_t> compressLzss(const std::vector<uint8_t> &data, bool vramCompatible, bool lz11Compression);

    /// @brief Compress input data using LZSS variant 10
    /// Compatible with : https://problemkaputt.de/gbatek.htm#biosdecompressionfunctions
    auto encodeLZ10(const std::vector<uint8_t> &data, bool vramCompatible = false) -> std::vector<uint8_t>;

    /// @brief Decompress input data using LZSS variant 10
    auto decodeLZ10(const std::vector<uint8_t> &data, bool vramCompatible = false) -> std::vector<uint8_t>;
}

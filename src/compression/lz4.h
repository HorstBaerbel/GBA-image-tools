#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Compression
{
    /// @brief Compress input data using LZ4 variant 40h
    /// @note This is probably not 100% stream compatible with regular LZ4
    auto encodeLZ4_40(const std::vector<uint8_t> &data, bool vramCompatible = false) -> std::vector<uint8_t>;

    /// @brief Decompress input data using LZ4 variant 40h
    auto decodeLZ4_40(const std::vector<uint8_t> &data, bool vramCompatible = false) -> std::vector<uint8_t>;
}

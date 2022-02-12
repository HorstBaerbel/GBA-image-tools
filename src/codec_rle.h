#pragma once

#include <cstdint>
#include <vector>

class RLE
{
public:
    /// @brief Compress input data using RLE and return the data
    static auto compressRLE(const std::vector<uint8_t> &data, bool /*vramCompatible*/) -> std::vector<uint8_t>;
};
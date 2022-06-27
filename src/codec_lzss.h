#pragma once

#include <cstdint>
#include <vector>

class LZSS
{
public:
    /// @brief Compress input data using LZSS compatible with GBA bios and return the data
    static auto encodeLZSS(const std::vector<uint8_t> &data, bool /*vramCompatible*/) -> std::vector<uint8_t>;
};
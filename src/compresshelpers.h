#pragma once

#include <string>
#include <vector>

namespace Compression
{

    /// @brief Find path to gbalzss executable
    /// @return Path if found, empty string if not found
    std::string findGbalzss();

    /// @brief Compress input data using lzss variant 10 or 11 and return the data
    std::vector<uint8_t> compressLzss(const std::vector<uint8_t> &data, bool vramCompatible, bool lz11Compression);

}

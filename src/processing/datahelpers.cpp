#include "datahelpers.h"

std::vector<uint8_t> interleave(const std::vector<std::vector<uint8_t>> &data, uint32_t bitsPerPixel)
{
    // make sure all vectors have the same sizes
    for (const auto &d : data)
    {
        REQUIRE(d.size() == data.front().size(), std::runtime_error, "All data sets to be interleaved must have the same size");
    }
    std::vector<uint8_t> result;
    if (bitsPerPixel == 4)
    {
        REQUIRE((data.size() & 1) == 0, std::runtime_error, "If interleave bit depth is 4, an even number of data sets must be passed!");
        for (uint32_t pi = 0; pi < data.front().size(); pi++)
        {
            // get all low nibbles
            for (uint32_t di = 0; di < data.size(); di += 2)
            {
                uint8_t lowNibbles = (data[di][pi] & 0x0F) | ((data[di + 1][pi] & 0x0F) << 4);
                result.push_back(lowNibbles);
            }
            // get all high nibbles
            for (uint32_t di = 0; di < data.size(); di += 2)
            {
                uint8_t highNibbles = ((data[di][pi] & 0xF0) >> 4) | (data[di + 1][pi] & 0xF0);
                result.push_back(highNibbles);
            }
        }
    }
    else if (bitsPerPixel == 8)
    {
        for (uint32_t pi = 0; pi < data.front().size(); pi++)
        {
            for (const auto &d : data)
            {
                result.push_back(d[pi]);
            }
        }
    }
    else if (bitsPerPixel == 15 || bitsPerPixel == 16)
    {
        REQUIRE((data.front().size() & 1) == 0, std::runtime_error, "If interleave bit depth is 15 or 16, data size must be even!");
        for (uint32_t pi = 0; pi < data.front().size(); pi += 2)
        {
            for (const auto &d : data)
            {
                result.push_back(d[pi]);
                result.push_back(d[pi + 1]);
            }
        }
    }
    else
    {
        THROW(std::runtime_error, "Bits per pixel must be 4, 8, 15 or 16!");
    }
    return result;
}

#include "datahelpers.h"

std::vector<uint8_t> interleave(const std::vector<std::vector<uint8_t>> &data, uint32_t bitsPerPixel)
{
    // make sure all vectors have the same sizes
    for (const auto &d : data)
    {
        if (d.size() != data.front().size())
        {
            throw std::runtime_error("All images must have the same number of pixels!");
        }
    }
    std::vector<uint8_t> result;
    if (bitsPerPixel == 4)
    {
        if ((data.size() & 1) != 0)
        {
            throw std::runtime_error("If bits per pixel is 4, an even number of images must me passed!");
        }
        for (uint32_t pi = 0; pi < data.front().size(); pi++)
        {
            uint32_t shift = ((pi & 1) == 0) ? 0 : 4;
            for (uint32_t di = 0; di < data.size(); di += 2)
            {
                uint8_t v = ((data[di][pi] >> shift) & 0x0F) | (((data[di + 1][pi] >> shift) & 0x0F) >> 4);
                result.push_back(v);
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
        if ((data.front().size() & 1) != 0)
        {
            throw std::runtime_error("If bits per pixel is 16, an even number of pixels must be passed!");
        }
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
        throw std::runtime_error("Bits per pixel must be 4, 8 or 16!");
    }
    return result;
}

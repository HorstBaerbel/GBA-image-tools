#pragma once

#include "color/xrgb8888.h"

#include <cstdint>
#include <vector>

class GVID
{
public:
    /// @brief Compress image data to GBA video format similar to Cinepak. See: https://en.wikipedia.org/wiki/Cinepak and: https://multimedia.cx/mirror/cinepak.txt
    /// Compresses video to YCgCoR format with intra- and inter-frame compression
    /// Stores codebooks as a single 4*Y + 1*Cg + 1*Co entry atm
    static auto encodeGVID(const std::vector<Color::XRGB8888> &image, uint32_t width, uint32_t height, bool keyFrame = true, float maxBlockError = 1.0F) -> std::vector<uint8_t>;

    /// @brief Decompress from GBA video format.
    static auto decodeGVID(const std::vector<uint8_t> &data, uint32_t width, uint32_t height) -> std::vector<Color::XRGB8888>;

private:
};

#pragma once

#include <cstdint>
#include <vector>

class DXTV
{
public:
    /// @brief Compress image data to format similar to DXT1. See: https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format
    // DXT1 compresses one 4x4 block to 2 bytes color0, 2 bytes color1 and 16*2 bit = 4 bytes index information
    /// Differences:
    /// - Colors will be stored as RGB555 only
    /// - Blocks are stored sequentially from left to right, top to bottom, but colors and indices are stored separately. First all colors, then all indices
    static auto encodeDXTV(const std::vector<uint16_t> &image, uint32_t width, uint32_t height, bool keyFrame, float maxBlockError) -> std::vector<uint8_t>;

    /// @brief Decompress from DXTV format.
    static auto decodeDXTV(const std::vector<uint8_t> &data, uint32_t width, uint32_t height) -> std::vector<uint8_t>;
};

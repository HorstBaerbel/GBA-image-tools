#pragma once

#include "color/xrgb8888.h"

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
    /// @param keyframe If true B-frame will be output, else a P-frame
    /// @param maxBlockError Max. allowed error for block references, if above a verbatim block will be stored. Range [0.01,1]
    /// @return Returns (compressed data, decompressed frame)
    static auto encode(const std::vector<Color::XRGB8888> &image, const std::vector<Color::XRGB8888> &previousImage, uint32_t width, uint32_t height, bool keyFrame, float maxBlockError, const bool swapToBGR = false) -> std::pair<std::vector<uint8_t>, std::vector<Color::XRGB8888>>;

    /// @brief Decompress from DXTV format
    static auto decode(const std::vector<uint8_t> &data, const std::vector<Color::XRGB8888> &previousImage, uint32_t width, uint32_t height) -> std::vector<Color::XRGB8888>;
};

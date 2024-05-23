#pragma once

#include "color/xrgb8888.h"

#include <cstdint>
#include <vector>

class DXT
{
public:
    /// @brief Compress 4x4, 8x8 or 16x16 block of image data to format similar to DXT1. See: https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format
    /// DXT1 compresses one 4x4 block to 2 bytes color0, 2 bytes color1 and 16*2 bit = 4 bytes index information
    /// Colors can be stored as XRGB1555 or RGB565
    template <unsigned DIMENSION>
    static auto encodeBlock(const std::vector<Color::XRGB8888> &block, const bool asRGB565, const bool swapToBGR) -> std::vector<uint8_t>;

    /// @brief Compress image data to format similar to DXT1. See: https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format
    /// DXT1 compresses one 4x4 block to 2 bytes color0, 2 bytes color1 and 16*2 bit = 4 bytes index information
    /// Colors can be stored as XRGB1555 or RGB565
    static auto encode(const std::vector<Color::XRGB8888> &image, uint32_t width, uint32_t height, bool asRGB565 = false, const bool swapToBGR = false) -> std::vector<uint8_t>;

    /// @brief Decompress 4x4, 8x8 or 16x16 block of DXT data
    template <unsigned DIMENSION>
    static auto decodeBlock(const std::vector<uint8_t> &data, const bool asRGB565, const bool swapToBGR) -> std::vector<Color::XRGB8888>;

    /// @brief Decompress image from DXT data
    static auto decode(const std::vector<uint8_t> &data, uint32_t width, uint32_t height, bool asRGB565 = false, const bool swapToBGR = false) -> std::vector<Color::XRGB8888>;
};

#pragma once

#include "color/xrgb8888.h"
#include "if/dxt_tables.h"

#include <cstdint>
#include <vector>

class DXT
{
public:
    /// @brief Get DXT colors from source, calculate intermediate colors and write to colors array
    /// @param data Pointer to start of DXT block data
    /// @param colors Pointer to an array of the 4 DXT block BGR555 colors. Must be word-aligned!
    /// @return Pointer past DXT block colors (data + 2)
    static inline __attribute__((always_inline)) auto getBlockColors(const uint16_t *data, uint16_t *colors) -> const uint16_t *
    {
        // get anchor colors c0 and c1
        uint32_t c0 = *data++;
        uint32_t c1 = *data++;
        colors[0] = c0;
        colors[1] = c1;
        uint32_t b = ((c0 & 0x7C00) >> 5) | ((c1 & 0x7C00) >> 10);
        uint32_t g = (c0 & 0x03E0) | ((c1 & 0x03E0) >> 5);
        uint32_t r = ((c0 & 0x001F) << 5) | (c1 & 0x001F);
        auto c2c3Ptr = reinterpret_cast<uint32_t *>(&colors[2]);
        // check which intermediate colors mode we're using
        if (c0 > c1)
        {
            // calculate intermediate colors c2 and c3 at 1/3 and 2/3 of c0 and c1
            *c2c3Ptr = (DXT_C2C3_ModeThird_5bit[b] << 10) | (DXT_C2C3_ModeThird_5bit[g] << 5) | DXT_C2C3_ModeThird_5bit[r];
        }
        else
        {
            // calculate intermediate color c2 at 1/2 of c0 and c1 with rounding and set c3 to black
            // uint32_t b = (((c0 & 0x7C00) >> 10) + ((c1 & 0x7C00) >> 10) + 1) >> 1;
            // uint32_t g = (((c0 & 0x03E0) >> 5) + ((c1 & 0x03E0) >> 5) + 1) >> 1;
            // uint32_t r = ((c0 & 0x001F) + (c1 & 0x001F) + 1) >> 1;
            //*c2c3Ptr = (b << 10) | (g << 5) | r;
            *c2c3Ptr = (static_cast<uint32_t>(DXT_C2_ModeHalf_5bit[b]) << 10) | (static_cast<uint32_t>(DXT_C2_ModeHalf_5bit[g]) << 5) | static_cast<uint32_t>(DXT_C2_ModeHalf_5bit[r]);
        }
        return data;
    }

    /// @brief Compress 4x4, 8x8 or 16x16 block of image data to format similar to DXT1. See: https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format
    /// DXT1 compresses one 4x4 block to 2 bytes color0, 2 bytes color1 and 16*2 bit = 4 bytes index information
    /// Colors can be stored as XRGB1555, XBGR1555, RGB565 or BGR565
    template <unsigned BLOCK_DIM>
    static auto encodeBlock(const std::vector<Color::XRGB8888> &block, const bool asRGB565, const bool swapToBGR) -> std::vector<uint8_t>;

    /// @brief Compress image data to format similar to DXT1. See: https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format
    /// DXT1 compresses one 4x4 block to 2 bytes color0, 2 bytes color1 and 16*2 bit = 4 bytes index information
    /// Colors can be stored as XRGB1555, XBGR1555, RGB565 or BGR565
    static auto encode(const std::vector<Color::XRGB8888> &image, uint32_t width, uint32_t height, bool asRGB565 = false, const bool swapToBGR = false) -> std::vector<uint8_t>;

    /// @brief Decompress 4x4, 8x8 or 16x16 block of DXT data
    template <unsigned BLOCK_DIM>
    static auto decodeBlock(const std::vector<uint8_t> &data, const bool asRGB565, const bool swapToBGR) -> std::vector<Color::XRGB8888>;

    /// @brief Decompress image from DXT data
    static auto decode(const std::vector<uint8_t> &data, uint32_t width, uint32_t height, bool asRGB565 = false, const bool swapToBGR = false) -> std::vector<Color::XRGB8888>;
};

#pragma once

#include <cstdint>

#include "codec/dxt_tables.h"
#include "sys/base.h"

namespace DXT
{

    /// @brief Get DXT colors from source, calculate intermediate colors and write to colors array
    /// @param data Pointer to start of DXT block data
    /// @param colors Pointer to an array of the 4 DXT block BGR555 colors. Must be word-aligned!
    /// @return Pointer past DXT block colors (data + 2)
    FORCEINLINE auto getBlockColors(const uint16_t *data, uint16_t *colors) -> const uint16_t *
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

    // Decompresses one DXT1-ish 4x4 block consisting of 2 bytes color0 (RGB555), 2 bytes color1 (RGB555) and 16*2 bit = 4 bytes index information
    // See: https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format
    // Blocks are stored sequentially from left to right, top to bottom, but colors and
    // indices are stored separately for better compression (first all colors, then all indices)
    // See also: https://stackoverflow.com/questions/56474930/efficiently-implementing-dxt1-texture-decompression-in-hardware
    template <uint32_t RESOLUTION_X>
    void UnCompWrite16bit(uint16_t *dst, const uint16_t *src, uint32_t width, uint32_t height);

}

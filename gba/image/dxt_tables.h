#pragma once

#include "memory/memory.h"

#include <cstdint>

namespace DXT
{

    /// @brief Lookup table for a 5-bit RGB color component (c0 << 5 | c1) that returns (c2 << 16 | c3)
    /// Formula: (round((2.0*floor(x/32)+(x%32))/3.0)) | (round((floor(x/32)+2.0*(x%32))/3.0)<<16), x in [0,32*32]
    // Generated using: https://horstbaerbel.github.io/jslut/index.html?equation=(round((2.0*floor(x%2F32)%2B(x%2532))%2F3.0))%20%7C%20(round((floor(x%2F32)%2B2.0*(x%2532))%2F3.0)%3C%3C16)&xstart=0&xend=(31%3C%3C5)|31&count=32*32&bitdepth=uint32_t&format=base10
    extern const uint32_t C2C3_ModeThird_5bit[1024];

    /// @brief Lookup table for a 5-bit RGB color component (c0 << 5 | c1) that returns (c2 << 16 | c3)
    /// Formula: (round((2.0*floor(x/32)+(x%32))/3.0)) | (round((floor(x/32)+2.0*(x%32))/3.0)<<16), x in [0,32*32]
    // Generated using: https://horstbaerbel.github.io/jslut/index.html?equation=round((floor(x%2F32)%2B(x%2532))%2F2.0)&xstart=0&xend=(31%3C%3C5)|31&count=32*32&bitdepth=uint16_t&format=base10
    extern const uint8_t C2_ModeHalf_5bit[1024];

    /// @brief Get DXT colors from source, calculate intermediate colors and write to DxtColors array
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
            *c2c3Ptr = (DXT::C2C3_ModeThird_5bit[b] << 10) | (DXT::C2C3_ModeThird_5bit[g] << 5) | DXT::C2C3_ModeThird_5bit[r];
        }
        else
        {
            // calculate intermediate color c2 at 1/2 of c0 and c1 with rounding and set c3 to black
            *c2c3Ptr = (DXT::C2_ModeHalf_5bit[b] << 10) | (DXT::C2_ModeHalf_5bit[g] << 5) | DXT::C2_ModeHalf_5bit[r];
        }
        return data;
    }
}

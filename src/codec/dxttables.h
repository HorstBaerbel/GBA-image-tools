#pragma once

#include <cstdint>

namespace DXTTables
{
    /// @brief Lookup table for a 5-bit RGB color component (c0 << 5 | c1) that returns (c2 << 16 | c3)
    /// Formula: (round((2.0*floor(x/32)+(x%32))/3.0)) | (round((floor(x/32)+2.0*(x%32))/3.0)<<16), x in [0,32*32]
    /// See: https://horstbaerbel.github.io/jslut/index.html?equation=16384%2Fx&xstart=0&xend=159&count=160&bitdepth=int16_t&format=base10
    extern const uint32_t C2C3_ModeThird_5bit[1024];

    /// @brief Lookup table for a 6-bit RGB color component (c0 << 5 | c1) that returns (c2 << 16 | c3)
    /// Formula: (round((2.0*floor(x/64)+(x%64))/3.0)) | (round((floor(x/64)+2.0*(x%64))/3.0)<<16), x in [0,64*64]
    extern const uint32_t C2C3_ModeThird_6bit[4096];
}

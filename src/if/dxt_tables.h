#pragma once

#include <cstdint>

/// @brief Intermediate IWRAM DXT block color storage
extern "C" uint16_t DXT_BlockColors[4];

/// @brief Lookup table for a 5-bit RGB color component (c0 << 5 | c1) that returns (c2 << 16 | c3)
/// Formula: (round((2.0*floor(x/32)+(x%32))/3.0)) | (round((floor(x/32)+2.0*(x%32))/3.0)<<16), x in [0,32*32]
// Generated using: https://horstbaerbel.github.io/jslut/index.html?equation=(round((2.0*floor(x%2F32)%2B(x%2532))%2F3.0))%20%7C%20(round((floor(x%2F32)%2B2.0*(x%2532))%2F3.0)%3C%3C16)&xstart=0&xend=(31%3C%3C5)|31&count=32*32&bitdepth=uint32_t&format=base10
extern "C" const uint32_t DXT_C2C3_ModeThird_5bit[1024];

/// @brief Lookup table for a 5-bit RGB color component (c0 << 5 | c1) that returns (c2 << 16 | c3)
/// Formula: round((floor(x/32)+(x%32))/2.0), x in [0,32*32]
// Generated using: https://horstbaerbel.github.io/jslut/index.html?equation=round((floor(x%2F32)%2B(x%2532))%2F2.0)&xstart=0&xend=(31%3C%3C5)|31&count=32*32&bitdepth=uint16_t&format=base10
extern "C" const uint8_t DXT_C2_ModeHalf_5bit[1024];

#ifndef __arm__
/// @brief Lookup table for a 6-bit RGB color component (c0 << 6 | c1) that returns (c2 << 16 | c3)
/// Formula: (round((2.0*floor(x/64)+(x%64))/3.0)) | (round((floor(x/64)+2.0*(x%64))/3.0)<<16), x in [0,64*64]
// Generated using: https://horstbaerbel.github.io/jslut/index.html?equation=(round((2.0*floor(x%2F64)%2B(x%2564))%2F3.0))%20%7C%20(round((floor(x%2F64)%2B2.0*(x%2564))%2F3.0)%3C%3C16)&xstart=0&xend=(63%3C%3C6)|63&count=64*64&bitdepth=uint32_t&format=base10
extern "C" const uint32_t DXT_C2C3_ModeThird_6bit[4096];
#endif

#pragma once

#ifndef __ASSEMBLER__
// LZ4 frame format constants for including in C++ files
#include <cstdint>

namespace Compression::Lz4Constants
{
    constexpr uint8_t TYPE_MARKER = 0x40;          // Used to detect LZ4 compression in data
    constexpr uint32_t MIN_MATCH_LENGTH = 4;       // A match needs at least 3 bytes to encode, thus 4 is the minimum match length
    constexpr uint32_t MAX_MATCH_LENGTH = 528;     // We want max. 4+8+8 bits to encode match length -> 528 = 15+255+255 + (MIN_MATCH_LENGTH - 1)
    constexpr uint32_t MAX_MATCH_DISTANCE = 65535; // We have max. 16 bits to encode match distance
    constexpr uint8_t LITERAL_LENGTH_SHIFT = 4;
    constexpr uint8_t LENGTH_MASK = 0x0F;
}

#else
// LZ4 frame format constants for including in assembly files
#define LZ4_CONSTANTS_TYPE_MARKER 0x40         // Used to detect LZ4 compression in data
#define LZ4_CONSTANTS_MIN_MATCH_LENGTH 4       // A match needs at least 3 bytes to encode, thus 4 is the minimum match length
#define LZ4_CONSTANTS_MAX_MATCH_LENGTH 528     // We want max. 4+8+8 bits to encode match length -> 528 = 15+255+255 + (LZ4_CONSTANTS_MIN_MATCH_LENGTH - 1)
#define LZ4_CONSTANTS_MAX_MATCH_DISTANCE 65535 // We have max. 16 bits to encode match distance
#define LZ4_CONSTANTS_LITERAL_LENGTH_SHIFT 4
#define LZ4_CONSTANTS_LENGTH_MASK 0x0F

#endif
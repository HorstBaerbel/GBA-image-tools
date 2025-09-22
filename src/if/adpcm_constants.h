#pragma once

#ifndef __ASSEMBLER__
// ADPCM frame format constants for including in C++ files
#include <cstdint>

namespace Audio::AdpcmConstants
{
    static constexpr uint32_t BITS_PER_SAMPLE = 4; // ADPCM bits per sample
    static constexpr uint32_t LOOKAHEAD = 3;       // Lookahead value for ADPCM encoder. The more the better quality, but slower encoding
}

#else
// ADPCM frame format constants for including in assembly files
#define ADPCM_CONSTANTS_BITS_PER_SAMPLE 4 // ADPCM bits per sample
#define ADPCM_CONSTANTS_LOOKAHEAD 3       // Lookahead value for ADPCM encoder. The more the better quality, but slower encoding

#endif
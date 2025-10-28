#pragma once

#ifndef __ASSEMBLER__
#include "adpcm_constants.h"

#include <cstdint>

namespace Audio
{
    /// @brief Frame header for one ADPCM frame
    /// ADPCM samples are encoded planar / per channel, e.g. L0 L1 ... R0 R1 ...
    struct AdpcmFrameHeader
    {
        uint16_t flags : 5;              // Flags (currently unused)
        uint16_t nrOfChannels : 2;       // PCM output channels [1,2]
        uint16_t pcmBitsPerSample : 6;   // PCM output sample bit depth in [1,32]
        uint16_t adpcmBitsPerSample : 3; // ADPCM sample bit depth in [3,5]
        uint16_t uncompressedSize = 0;   // Uncompressed size of PCM output data

        static auto write(uint32_t *dst, const AdpcmFrameHeader &header) -> void;
        static auto read(const uint32_t *src) -> AdpcmFrameHeader;
    } __attribute__((aligned(4), packed));
}
#else

#define ADPCM_FRAMEHEADER_SIZE 4

#endif

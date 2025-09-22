#pragma once

#include "adpcm_constants.h"

#include <cstdint>
#include <vector>

namespace Audio
{
    /// @brief Frame header for one ADPCM frame
    struct AdpcmFrameHeader
    {
        uint16_t flags : 5 = 0;                                            // Flags (currently unused)
        uint16_t nrOfChannels : 2 = 0;                                     // PCM output channels [1,2]
        uint16_t pcmBitsPerSample : 6 = 16;                                // PCM output sample bit depth in [1,32]
        uint16_t adpcmBitsPerSample : 3 = AdpcmConstants::BITS_PER_SAMPLE; // ADPCM sample bit depth in [3,5]
        uint16_t uncompressedSize = 0;                                     // Uncompressed size of PCM output data

        std::vector<uint8_t> toVector() const;
        static auto fromVector(const std::vector<uint8_t> &data) -> AdpcmFrameHeader;
    } __attribute__((aligned(4), packed));
}

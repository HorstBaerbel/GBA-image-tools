#include "adpcm.h"

namespace ADPCM
{
    void UnCompWrite32bit(const uint32_t *data, uint32_t *dst, uint32_t sampleBits)
    {
    }

    uint32_t UnCompGetSize(const uint32_t *data, uint32_t sampleBits)
    {
        auto dataPtr16 = reinterpret_cast<const uint16_t *>(data);
        const auto data16 = *dataPtr16++;
        FrameHeader header;
        header.flags = (data16 & 0x1F);
        header.nrOfChannels = (data16 & 0x60) >> 5;
        header.pcmBitsPerSample = (data16 & 0x1F80) >> 7;
        header.pcmBitsPerSample = (data16 & 0xE000) >> 13;
        header.uncompressedSize = *dataPtr16;
    }
}

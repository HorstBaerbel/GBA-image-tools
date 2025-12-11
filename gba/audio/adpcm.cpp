#include "adpcm.h"

#include "adpcm_structs.h"
#include "adpcm_tables.h"

#define ADPCM_DITHER
#define ADPCM_DITHER_SHIFT 24
// #define ADPCM_ROUNDING

namespace Adpcm
{
    void IWRAM_FUNC UnCompWrite32bit_8bit(const uint32_t *data, uint32_t dataSize, uint32_t *dst)
    {
        //  copy frame header and skip to data
        const Audio::AdpcmFrameHeader frameHeader = Audio::AdpcmFrameHeader::read(data);
        auto data8 = reinterpret_cast<const uint8_t *>(data + sizeof(Audio::AdpcmFrameHeader) / 4);
        // uncompress 4-bit ADPCM samples. These are stored planar / per channel, e.g. L0 L1 ... R0 R1 ...
        const auto adpcmDataSize = dataSize - sizeof(Audio::AdpcmFrameHeader);
        auto dst8 = reinterpret_cast<uint8_t *>(dst);
        const auto adpcmChannelBlockSize = adpcmDataSize / frameHeader.nrOfChannels;
        for (uint32_t channel = 0; channel < frameHeader.nrOfChannels; ++channel)
        {
            // align output buffer to next word boundary
            dst8 = reinterpret_cast<uint8_t *>((reinterpret_cast<uint32_t>(dst8) + 3) & 0xFFFFFFFC);
            // first sample is stored verbatim in header
            int32_t pcmData = *reinterpret_cast<const int16_t *>(data8);
#ifdef ADPCM_DITHER
            pcmData += (ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT) - ADPCM_DitherState[0];
            ADPCM_DitherState[0] = ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT;
            ADPCM_DitherState[1] = ((ADPCM_DitherState[1] << 4) - ADPCM_DitherState[1]) ^ 1;
            pcmData = pcmData < -32768 ? -32768 : pcmData;
            pcmData = pcmData > 32767 ? 32767 : pcmData;
#endif
#ifdef ADPCM_ROUNDING
            *dst8++ = (pcmData + 128) >> 8;
#else
            *dst8++ = pcmData >> 8;
#endif
            int32_t index = *reinterpret_cast<const int16_t *>(data8 + 2);
            data8 += 4;
            uint32_t bytesLeft = adpcmChannelBlockSize - 4;
            while (bytesLeft--)
            {
                // get two ADPCM nibbles
                const uint32_t nibbles = *data8;
                // decode first nibble
                uint32_t delta = ADPCM_DeltaTable_4bit[index][nibbles & 0x07];
                if (nibbles & 8)
                    pcmData -= delta;
                else
                    pcmData += delta;
                index += ADPCM_IndexTable_4bit[nibbles & 0x07];
                index = index < 0 ? 0 : index;
                index = index > 88 ? 88 : index;
#ifdef ADPCM_DITHER
                pcmData += (ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT) - ADPCM_DitherState[0];
                ADPCM_DitherState[0] = ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT;
                ADPCM_DitherState[1] = ((ADPCM_DitherState[1] << 4) - ADPCM_DitherState[1]) ^ 1;
#endif
                pcmData = pcmData < -32768 ? -32768 : pcmData;
                pcmData = pcmData > 32767 ? 32767 : pcmData;
#ifdef ADPCM_ROUNDING
                *dst8++ = (pcmData + 128) >> 8;
#else
                *dst8++ = pcmData >> 8;
#endif
                // decode second nibble only if not last sample
                if (bytesLeft > 0)
                {
                    delta = ADPCM_DeltaTable_4bit[index][(nibbles >> 4) & 0x07];
                    if ((nibbles >> 4) & 8)
                        pcmData -= delta;
                    else
                        pcmData += delta;
                    index += ADPCM_IndexTable_4bit[(nibbles >> 4) & 0x07];
                    index = index < 0 ? 0 : index;
                    index = index > 88 ? 88 : index;
#ifdef ADPCM_DITHER
                    pcmData += (ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT) - ADPCM_DitherState[0];
                    ADPCM_DitherState[0] = ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT;
                    ADPCM_DitherState[1] = ((ADPCM_DitherState[1] << 4) - ADPCM_DitherState[1]) ^ 1;
#endif
                    pcmData = pcmData < -32768 ? -32768 : pcmData;
                    pcmData = pcmData > 32767 ? 32767 : pcmData;
#ifdef ADPCM_ROUNDING
                    *dst8++ = (pcmData + 128) >> 8;
#else
                    *dst8++ = pcmData >> 8;
#endif
                }
                // advance input data
                data8++;
            }
        }
    }

    uint32_t IWRAM_FUNC UnCompGetSize_8bit(const uint32_t *data)
    {
        const Audio::AdpcmFrameHeader frameHeader = Audio::AdpcmFrameHeader::read(data);
        // if we're down-converting the PCM sample depth during decompression, adjust the uncompressed data size too
        return (static_cast<uint32_t>(frameHeader.uncompressedSize) * 8 + 7) / frameHeader.pcmBitsPerSample; // + frameHeader.nrOfChannels;
    }
}

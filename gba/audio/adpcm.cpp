#include "adpcm.h"

#include "adpcm_asm.h"
#include "if/adpcm_structs.h"
#include "if/adpcm_tables.h"

#define USE_ASM

#define ADPCM_DITHER
#define ADPCM_DITHER_SHIFT 24

namespace Adpcm
{

    template <>
    void IWRAM_FUNC UnCompWrite32bit<8>(const uint32_t *data, uint32_t dataSize, uint32_t *dst)
    {
#ifdef USE_ASM
        UnCompWrite32bit_8bit(data, dataSize, dst);
#else
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
            // dst8 = reinterpret_cast<uint8_t *>((reinterpret_cast<uint32_t>(dst8) + 3) & 0xFFFFFFFC);
            // first sample is stored verbatim in header
            int32_t pcmData = *reinterpret_cast<const int16_t *>(data8);
#ifdef ADPCM_DITHER
            pcmData += (ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT) - ADPCM_DitherState[0];
            ADPCM_DitherState[0] = ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT;
            ADPCM_DitherState[1] = ((ADPCM_DitherState[1] << 4) - ADPCM_DitherState[1]) ^ 1;
            pcmData = pcmData < -32768 ? -32768 : pcmData;
            pcmData = pcmData > 32767 ? 32767 : pcmData;
#endif
            *dst8++ = pcmData >> 8;
            int32_t index = *reinterpret_cast<const int16_t *>(data8 + 2);
            data8 += 4;
            uint32_t bytesLeft = adpcmChannelBlockSize - 4;
            while (bytesLeft--)
            {
                // get two ADPCM nibbles
                const uint32_t nibbles = *data8;
                // decode first nibble
                uint32_t step = ADPCM_StepTable[index];
                uint32_t delta = step >> 3;
                if (nibbles & 1)
                    delta += (step >> 2);
                if (nibbles & 2)
                    delta += (step >> 1);
                if (nibbles & 4)
                    delta += step;
                if (nibbles & 8)
                    pcmData -= delta;
                else
                    pcmData += delta;
                index += ADPCM_IndexTable_4bit[nibbles & 0x7];
                index = index < 0 ? 0 : index;
                index = index > 88 ? 88 : index;
#ifdef ADPCM_DITHER
                pcmData += (ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT) - ADPCM_DitherState[0];
                ADPCM_DitherState[0] = ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT;
                ADPCM_DitherState[1] = ((ADPCM_DitherState[1] << 4) - ADPCM_DitherState[1]) ^ 1;
#endif
                pcmData = pcmData < -32768 ? -32768 : pcmData;
                pcmData = pcmData > 32767 ? 32767 : pcmData;
                *dst8++ = pcmData >> 8;
                // decode second nibble only if not last sample
                if (bytesLeft > 0)
                {
                    step = ADPCM_StepTable[index];
                    delta = step >> 3;
                    if (nibbles & 0x10)
                        delta += (step >> 2);
                    if (nibbles & 0x20)
                        delta += (step >> 1);
                    if (nibbles & 0x40)
                        delta += step;
                    if (nibbles & 0x80)
                        pcmData -= delta;
                    else
                        pcmData += delta;
                    index += ADPCM_IndexTable_4bit[(nibbles >> 4) & 0x7];
                    index = index < 0 ? 0 : index;
                    index = index > 88 ? 88 : index;
#ifdef ADPCM_DITHER
                    pcmData += (ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT) - ADPCM_DitherState[0];
                    ADPCM_DitherState[0] = ADPCM_DitherState[1] >> ADPCM_DITHER_SHIFT;
                    ADPCM_DitherState[1] = ((ADPCM_DitherState[1] << 4) - ADPCM_DitherState[1]) ^ 1;
#endif
                    pcmData = pcmData < -32768 ? -32768 : pcmData;
                    pcmData = pcmData > 32767 ? 32767 : pcmData;
                    *dst8++ = pcmData >> 8;
                }
                // advance input data
                data8++;
            }
        }
#endif
    }

    template <>
    uint32_t IWRAM_FUNC UnCompGetSize<8>(const uint32_t *data)
    {
#ifdef USE_ASM
        return UnCompGetSize_8bit(data);
#else
        const Audio::AdpcmFrameHeader frameHeader = Audio::AdpcmFrameHeader::read(data);
        // if we're down-converting the PCM sample depth during decompression, adjust the uncompressed data size too
        return (static_cast<uint32_t>(frameHeader.uncompressedSize) * 8 + 7) / frameHeader.pcmBitsPerSample; // + frameHeader.nrOfChannels;
#endif
    }
}

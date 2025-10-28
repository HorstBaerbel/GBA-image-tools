#include "adpcm.h"

#include "if/adpcm_structs.h"
#include "if/adpcm_tables.h"

#define DITHER
#define DITHER_SHIFT 24

namespace Adpcm
{

#ifdef DITHER
    static IWRAM_DATA ALIGN(4) int32_t dither = 0x159A55A0;
    static IWRAM_DATA ALIGN(4) int32_t lastDither = 0;
#endif

    template <>
    void IWRAM_FUNC UnCompWrite32bit<8>(const uint32_t *data, uint32_t dataSize, uint32_t *dst)
    {
        // copy frame header and skip to data
        const Audio::AdpcmFrameHeader frameHeader = Audio::AdpcmFrameHeader::read(data);
        data += sizeof(Audio::AdpcmFrameHeader) / 4;
        // uncompress 4-bit ADPCM samples. These are stored planar / per channel, e.g. L0 L1 ... R0 R1 ...
        const auto adpcmDataSize = dataSize - sizeof(Audio::AdpcmFrameHeader);
        auto data8 = reinterpret_cast<const uint8_t *>(data);
        auto dst8 = reinterpret_cast<uint8_t *>(dst);
        const auto adpcmChannelBlockSize = adpcmDataSize / frameHeader.nrOfChannels;
        for (uint32_t channel = 0; channel < frameHeader.nrOfChannels; ++channel)
        {
            // align output buffer to next word boundary
            dst8 = reinterpret_cast<uint8_t *>((reinterpret_cast<uint32_t>(dst8) + 3) & 0xFFFFFFFC);
            // first sample is stored verbatim in header
            int32_t pcmData = *reinterpret_cast<const int16_t *>(data8);
#ifdef DITHER
            pcmData += (dither >> DITHER_SHIFT) - lastDither;
            lastDither = dither >> DITHER_SHIFT;
            dither = ((dither << 4) - dither) ^ 1;
            pcmData = pcmData < -32768 ? -32768 : pcmData;
            pcmData = pcmData > 32767 ? 32767 : pcmData;
#endif
            *dst8++ = pcmData >> 8;
            int32_t index = data8[2];
            data8 += 4;
            uint32_t bytesLeft = adpcmChannelBlockSize - 4;
            while (bytesLeft--)
            {
                // decode first nibble
                uint32_t step = ADPCM_StepTable[index];
                uint32_t delta = step >> 3;
                if (*data8 & 1)
                    delta += (step >> 2);
                if (*data8 & 2)
                    delta += (step >> 1);
                if (*data8 & 4)
                    delta += step;
                if (*data8 & 8)
                    pcmData -= delta;
                else
                    pcmData += delta;
                index += ADPCM_IndexTable_4bit[*data8 & 0x7];
                index = index < 0 ? 0 : index;
                index = index > 88 ? 88 : index;
#ifdef DITHER
                pcmData += (dither >> DITHER_SHIFT) - lastDither;
                lastDither = dither >> DITHER_SHIFT;
                dither = ((dither << 4) - dither) ^ 1;
#endif
                pcmData = pcmData < -32768 ? -32768 : pcmData;
                pcmData = pcmData > 32767 ? 32767 : pcmData;
                *dst8++ = pcmData >> 8;
                // decode second nibble only if not last sample
                if (bytesLeft > 0)
                {
                    step = ADPCM_StepTable[index];
                    delta = step >> 3;
                    if (*data8 & 0x10)
                        delta += (step >> 2);
                    if (*data8 & 0x20)
                        delta += (step >> 1);
                    if (*data8 & 0x40)
                        delta += step;
                    if (*data8 & 0x80)
                        pcmData -= delta;
                    else
                        pcmData += delta;
                    index += ADPCM_IndexTable_4bit[(*data8 >> 4) & 0x7];
                    index = index < 0 ? 0 : index;
                    index = index > 88 ? 88 : index;
#ifdef DITHER
                    pcmData += (dither >> DITHER_SHIFT) - lastDither;
                    lastDither = dither >> DITHER_SHIFT;
                    dither = ((dither << 4) - dither) ^ 1;
#endif
                    pcmData = pcmData < -32768 ? -32768 : pcmData;
                    pcmData = pcmData > 32767 ? 32767 : pcmData;
                    *dst8++ = pcmData >> 8;
                }
                // advance input data
                data8++;
            }
        }
    }

    template <>
    uint32_t IWRAM_FUNC UnCompGetSize<8>(const uint32_t *data)
    {
        const Audio::AdpcmFrameHeader frameHeader = Audio::AdpcmFrameHeader::read(data);
        // if we're down-converting the PCM sample depth during decompression, adjust the uncompressed data size too
        return (static_cast<uint32_t>(frameHeader.uncompressedSize) * 8 + 7) / frameHeader.pcmBitsPerSample; // + frameHeader.nrOfChannels;
    }
}

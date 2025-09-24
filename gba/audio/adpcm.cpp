#include "adpcm.h"

#include "if/adpcm_structs.h"

namespace Adpcm
{

    /// @brief ADPCM step table
    IWRAM_DATA ALIGN(4) static const uint16_t StepTable[89] = {
        7, 8, 9, 10, 11, 12, 13, 14,
        16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66,
        73, 80, 88, 97, 107, 118, 130, 143,
        157, 173, 190, 209, 230, 253, 279, 307,
        337, 371, 408, 449, 494, 544, 598, 658,
        724, 796, 876, 963, 1060, 1166, 1282, 1411,
        1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
        3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
        7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
        15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
        32767};

    /// @brief ADPCM step index table for 4-bit indices
    IWRAM_DATA ALIGN(4) static const int32_t IndexTable_4bit[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

    template <>
    void UnCompWrite32bit<8>(const uint32_t *data, uint32_t dataSize, uint32_t *dst)
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
            *dst8++ = pcmData >> 8;
            int32_t index = data8[2];
            data8 += 4;
            uint32_t bytesLeft = adpcmChannelBlockSize - 4;
            while (bytesLeft--)
            {
                // decode first nibble
                uint32_t step = StepTable[index];
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
                index += IndexTable_4bit[*data8 & 0x7];
                index = index < 0 ? 0 : index;
                index = index > 88 ? 88 : index;
                pcmData = pcmData < -32768 ? -32768 : pcmData;
                pcmData = pcmData > 32767 ? 32767 : pcmData;
                *dst8++ = pcmData >> 8;
                // decode second nibble only if not last sample
                if (bytesLeft > 0)
                {
                    step = StepTable[index];
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
                    index += IndexTable_4bit[(*data8 >> 4) & 0x7];
                    index = index < 0 ? 0 : index;
                    index = index > 88 ? 88 : index;
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
    uint32_t UnCompGetSize<8>(const uint32_t *data)
    {
        const Audio::AdpcmFrameHeader frameHeader = Audio::AdpcmFrameHeader::read(data);
        // if we're down-converting the PCM sample depth during decompression, adjust the uncompressed data size too
        return static_cast<uint32_t>(frameHeader.uncompressedSize) * 8 / frameHeader.pcmBitsPerSample; // + frameHeader.nrOfChannels;
    }
}

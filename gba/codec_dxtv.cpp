#include "codec_dxtv.h"

#include "memory.h"
#include "output.h"

namespace DXTV
{

    constexpr uint16_t FRAME_IS_PFRAME = 0x80; // 0 for key frames, 1 for inter-frame compression ("predicted frame")
    constexpr uint32_t BLOCK_PREVIOUS = 0x01;  // The block is from from the previous frame
    constexpr uint32_t BLOCK_REFERENCE = 0x02; // The block is a reference into the current or previous frame

    // Block flags mean:
    // 0 | 0 --> new, full DXT block
    // 0 | BLOCK_REFERENCE --> reference into current frame
    // BLOCK_PREVIOUS | BLOCK_REFERENCE --> reference into previous frame
    // BLOCK_PREVIOUS | 0 --> keep previous frame block

    IWRAM_DATA struct FrameHeader
    {
        uint16_t flags = 0;         // e.g. FRAME_IS_PFRAME
        uint16_t nrOfRefBlocks = 0; // # of reference blocks in frame. rest is verbatim DXT blocks
    } ALIGN_PACK(4) frameHeader;

    /// @brief Lookup table for c0 vs c1 that returns (c2 << 16 | c3)
    /// Formula: (round((2.0*floor(i/32)+(i%32))/3.0)) | (round((floor(i/32)+2.0*(i%32))/3.0)<<16), i in [0,32*32]
    IWRAM_DATA ALIGN(4) const uint32_t C2C3table[1024] = {
        0, 65536, 65537, 131073, 196609, 196610, 262146, 327682, 327683, 393219, 458755,
        458756, 524292, 589828, 589829, 655365, 720901, 720902, 786438, 851974, 851975,
        917511, 983047, 983048, 1048584, 1114120, 1114121, 1179657, 1245193, 1245194, 1310730,
        1376266, 1, 65537, 131073, 131074, 196610, 262146, 262147, 327683, 393219,
        393220, 458756, 524292, 524293, 589829, 655365, 655366, 720902, 786438, 786439,
        851975, 917511, 917512, 983048, 1048584, 1048585, 1114121, 1179657, 1179658, 1245194,
        1310730, 1310731, 1376267, 65537, 65538, 131074, 196610, 196611, 262147, 327683,
        327684, 393220, 458756, 458757, 524293, 589829, 589830, 655366, 720902, 720903,
        786439, 851975, 851976, 917512, 983048, 983049, 1048585, 1114121, 1114122, 1179658,
        1245194, 1245195, 1310731, 1376267, 1376268, 65538, 131074, 131075, 196611, 262147,
        262148, 327684, 393220, 393221, 458757, 524293, 524294, 589830, 655366, 655367,
        720903, 786439, 786440, 851976, 917512, 917513, 983049, 1048585, 1048586, 1114122,
        1179658, 1179659, 1245195, 1310731, 1310732, 1376268, 1441804, 65539, 131075, 196611,
        196612, 262148, 327684, 327685, 393221, 458757, 458758, 524294, 589830, 589831,
        655367, 720903, 720904, 786440, 851976, 851977, 917513, 983049, 983050, 1048586,
        1114122, 1114123, 1179659, 1245195, 1245196, 1310732, 1376268, 1376269, 1441805, 131075,
        131076, 196612, 262148, 262149, 327685, 393221, 393222, 458758, 524294, 524295,
        589831, 655367, 655368, 720904, 786440, 786441, 851977, 917513, 917514, 983050,
        1048586, 1048587, 1114123, 1179659, 1179660, 1245196, 1310732, 1310733, 1376269, 1441805,
        1441806, 131076, 196612, 196613, 262149, 327685, 327686, 393222, 458758, 458759,
        524295, 589831, 589832, 655368, 720904, 720905, 786441, 851977, 851978, 917514,
        983050, 983051, 1048587, 1114123, 1114124, 1179660, 1245196, 1245197, 1310733, 1376269,
        1376270, 1441806, 1507342, 131077, 196613, 262149, 262150, 327686, 393222, 393223,
        458759, 524295, 524296, 589832, 655368, 655369, 720905, 786441, 786442, 851978,
        917514, 917515, 983051, 1048587, 1048588, 1114124, 1179660, 1179661, 1245197, 1310733,
        1310734, 1376270, 1441806, 1441807, 1507343, 196613, 196614, 262150, 327686, 327687,
        393223, 458759, 458760, 524296, 589832, 589833, 655369, 720905, 720906, 786442,
        851978, 851979, 917515, 983051, 983052, 1048588, 1114124, 1114125, 1179661, 1245197,
        1245198, 1310734, 1376270, 1376271, 1441807, 1507343, 1507344, 196614, 262150, 262151,
        327687, 393223, 393224, 458760, 524296, 524297, 589833, 655369, 655370, 720906,
        786442, 786443, 851979, 917515, 917516, 983052, 1048588, 1048589, 1114125, 1179661,
        1179662, 1245198, 1310734, 1310735, 1376271, 1441807, 1441808, 1507344, 1572880, 196615,
        262151, 327687, 327688, 393224, 458760, 458761, 524297, 589833, 589834, 655370,
        720906, 720907, 786443, 851979, 851980, 917516, 983052, 983053, 1048589, 1114125,
        1114126, 1179662, 1245198, 1245199, 1310735, 1376271, 1376272, 1441808, 1507344, 1507345,
        1572881, 262151, 262152, 327688, 393224, 393225, 458761, 524297, 524298, 589834,
        655370, 655371, 720907, 786443, 786444, 851980, 917516, 917517, 983053, 1048589,
        1048590, 1114126, 1179662, 1179663, 1245199, 1310735, 1310736, 1376272, 1441808, 1441809,
        1507345, 1572881, 1572882, 262152, 327688, 327689, 393225, 458761, 458762, 524298,
        589834, 589835, 655371, 720907, 720908, 786444, 851980, 851981, 917517, 983053,
        983054, 1048590, 1114126, 1114127, 1179663, 1245199, 1245200, 1310736, 1376272, 1376273,
        1441809, 1507345, 1507346, 1572882, 1638418, 262153, 327689, 393225, 393226, 458762,
        524298, 524299, 589835, 655371, 655372, 720908, 786444, 786445, 851981, 917517,
        917518, 983054, 1048590, 1048591, 1114127, 1179663, 1179664, 1245200, 1310736, 1310737,
        1376273, 1441809, 1441810, 1507346, 1572882, 1572883, 1638419, 327689, 327690, 393226,
        458762, 458763, 524299, 589835, 589836, 655372, 720908, 720909, 786445, 851981,
        851982, 917518, 983054, 983055, 1048591, 1114127, 1114128, 1179664, 1245200, 1245201,
        1310737, 1376273, 1376274, 1441810, 1507346, 1507347, 1572883, 1638419, 1638420, 327690,
        393226, 393227, 458763, 524299, 524300, 589836, 655372, 655373, 720909, 786445,
        786446, 851982, 917518, 917519, 983055, 1048591, 1048592, 1114128, 1179664, 1179665,
        1245201, 1310737, 1310738, 1376274, 1441810, 1441811, 1507347, 1572883, 1572884, 1638420,
        1703956, 327691, 393227, 458763, 458764, 524300, 589836, 589837, 655373, 720909,
        720910, 786446, 851982, 851983, 917519, 983055, 983056, 1048592, 1114128, 1114129,
        1179665, 1245201, 1245202, 1310738, 1376274, 1376275, 1441811, 1507347, 1507348, 1572884,
        1638420, 1638421, 1703957, 393227, 393228, 458764, 524300, 524301, 589837, 655373,
        655374, 720910, 786446, 786447, 851983, 917519, 917520, 983056, 1048592, 1048593,
        1114129, 1179665, 1179666, 1245202, 1310738, 1310739, 1376275, 1441811, 1441812, 1507348,
        1572884, 1572885, 1638421, 1703957, 1703958, 393228, 458764, 458765, 524301, 589837,
        589838, 655374, 720910, 720911, 786447, 851983, 851984, 917520, 983056, 983057,
        1048593, 1114129, 1114130, 1179666, 1245202, 1245203, 1310739, 1376275, 1376276, 1441812,
        1507348, 1507349, 1572885, 1638421, 1638422, 1703958, 1769494, 393229, 458765, 524301,
        524302, 589838, 655374, 655375, 720911, 786447, 786448, 851984, 917520, 917521,
        983057, 1048593, 1048594, 1114130, 1179666, 1179667, 1245203, 1310739, 1310740, 1376276,
        1441812, 1441813, 1507349, 1572885, 1572886, 1638422, 1703958, 1703959, 1769495, 458765,
        458766, 524302, 589838, 589839, 655375, 720911, 720912, 786448, 851984, 851985,
        917521, 983057, 983058, 1048594, 1114130, 1114131, 1179667, 1245203, 1245204, 1310740,
        1376276, 1376277, 1441813, 1507349, 1507350, 1572886, 1638422, 1638423, 1703959, 1769495,
        1769496, 458766, 524302, 524303, 589839, 655375, 655376, 720912, 786448, 786449,
        851985, 917521, 917522, 983058, 1048594, 1048595, 1114131, 1179667, 1179668, 1245204,
        1310740, 1310741, 1376277, 1441813, 1441814, 1507350, 1572886, 1572887, 1638423, 1703959,
        1703960, 1769496, 1835032, 458767, 524303, 589839, 589840, 655376, 720912, 720913,
        786449, 851985, 851986, 917522, 983058, 983059, 1048595, 1114131, 1114132, 1179668,
        1245204, 1245205, 1310741, 1376277, 1376278, 1441814, 1507350, 1507351, 1572887, 1638423,
        1638424, 1703960, 1769496, 1769497, 1835033, 524303, 524304, 589840, 655376, 655377,
        720913, 786449, 786450, 851986, 917522, 917523, 983059, 1048595, 1048596, 1114132,
        1179668, 1179669, 1245205, 1310741, 1310742, 1376278, 1441814, 1441815, 1507351, 1572887,
        1572888, 1638424, 1703960, 1703961, 1769497, 1835033, 1835034, 524304, 589840, 589841,
        655377, 720913, 720914, 786450, 851986, 851987, 917523, 983059, 983060, 1048596,
        1114132, 1114133, 1179669, 1245205, 1245206, 1310742, 1376278, 1376279, 1441815, 1507351,
        1507352, 1572888, 1638424, 1638425, 1703961, 1769497, 1769498, 1835034, 1900570, 524305,
        589841, 655377, 655378, 720914, 786450, 786451, 851987, 917523, 917524, 983060,
        1048596, 1048597, 1114133, 1179669, 1179670, 1245206, 1310742, 1310743, 1376279, 1441815,
        1441816, 1507352, 1572888, 1572889, 1638425, 1703961, 1703962, 1769498, 1835034, 1835035,
        1900571, 589841, 589842, 655378, 720914, 720915, 786451, 851987, 851988, 917524,
        983060, 983061, 1048597, 1114133, 1114134, 1179670, 1245206, 1245207, 1310743, 1376279,
        1376280, 1441816, 1507352, 1507353, 1572889, 1638425, 1638426, 1703962, 1769498, 1769499,
        1835035, 1900571, 1900572, 589842, 655378, 655379, 720915, 786451, 786452, 851988,
        917524, 917525, 983061, 1048597, 1048598, 1114134, 1179670, 1179671, 1245207, 1310743,
        1310744, 1376280, 1441816, 1441817, 1507353, 1572889, 1572890, 1638426, 1703962, 1703963,
        1769499, 1835035, 1835036, 1900572, 1966108, 589843, 655379, 720915, 720916, 786452,
        851988, 851989, 917525, 983061, 983062, 1048598, 1114134, 1114135, 1179671, 1245207,
        1245208, 1310744, 1376280, 1376281, 1441817, 1507353, 1507354, 1572890, 1638426, 1638427,
        1703963, 1769499, 1769500, 1835036, 1900572, 1900573, 1966109, 655379, 655380, 720916,
        786452, 786453, 851989, 917525, 917526, 983062, 1048598, 1048599, 1114135, 1179671,
        1179672, 1245208, 1310744, 1310745, 1376281, 1441817, 1441818, 1507354, 1572890, 1572891,
        1638427, 1703963, 1703964, 1769500, 1835036, 1835037, 1900573, 1966109, 1966110, 655380,
        720916, 720917, 786453, 851989, 851990, 917526, 983062, 983063, 1048599, 1114135,
        1114136, 1179672, 1245208, 1245209, 1310745, 1376281, 1376282, 1441818, 1507354, 1507355,
        1572891, 1638427, 1638428, 1703964, 1769500, 1769501, 1835037, 1900573, 1900574, 1966110,
        2031646, 655381, 720917, 786453, 786454, 851990, 917526, 917527, 983063, 1048599,
        1048600, 1114136, 1179672, 1179673, 1245209, 1310745, 1310746, 1376282, 1441818, 1441819,
        1507355, 1572891, 1572892, 1638428, 1703964, 1703965, 1769501, 1835037, 1835038, 1900574,
        1966110, 1966111, 2031647};

    IWRAM_DATA ALIGN(4) uint16_t colors[4];

    template <>
    IWRAM_FUNC void UnCompWrite16bit<240>(uint16_t *dst, const uint32_t *src, const uint32_t *prevSrc, uint32_t width, uint32_t height)
    {
        constexpr uint32_t LineStride16 = 240;                   // stride to next line in dst in half-words
        constexpr uint32_t LineStride32 = LineStride16 / 2;      // stride to next line in dst in words
        constexpr uint32_t BlockLineStride16 = LineStride16 * 4; // vertical stride to next block in dst (4 lines)
        constexpr uint32_t BlockStride16 = 4;                    // horizontal stride to next block in dst (4 * 2 bytes)
        const uint32_t nrOfBlocks = width / 4 * height / 4;
        uint32_t blockIndex = 0; // 4x4 block index in frame
        // copy frame header
        Memory::memcpy32(&frameHeader, src, sizeof(FrameHeader) / 4);
        const bool isKeyFrame = (frameHeader.flags & FRAME_IS_PFRAME) == 0;
        // set up some variables
        auto c2c3Ptr = reinterpret_cast<uint32_t *>(&colors[2]);
        auto srcFlagPtr = reinterpret_cast<const uint16_t *>(src + (sizeof(FrameHeader) / 4));              // flags that define encoding state of blocks
        uint32_t flags = 0;                                                                                 // flags for current 8 blocks
        auto srcRefPtr = reinterpret_cast<const uint8_t *>(srcFlagPtr) + ((2 * nrOfBlocks) / 8);            // reference block data. offset is 2 flag bits per block = 2 bytes per 8 blocks
        const uint32_t nrOfRefBlocks = (static_cast<uint32_t>(frameHeader.nrOfRefBlocks) + 3) & 0xFFFFFFFC; // round # of references up to multiple of 4
        auto srcDxtPtr = reinterpret_cast<const uint16_t *>(srcRefPtr + nrOfRefBlocks);                     // DXT block data. offset is nrOfRefBlocks bytes
        auto currentDst = dst;
        for (uint32_t blockY = 0; blockY < height / 4; blockY++)
        {
            auto blockLineDst = currentDst;
            for (uint32_t blockX = 0; blockX < width / 4; blockX++)
            {
                auto blockDst = blockLineDst;
                // load new block flags every 8 blocks
                if ((blockIndex & 0x07) == 0)
                {
                    flags = *srcFlagPtr++;
                }
                // check block encoding type
                if ((flags & BLOCK_REFERENCE) != 0)
                {
                    // block reference. get block index (# of blocks)
                    uint32_t refBlockIndex = (blockIndex - 1) - *srcRefPtr++;
                    // block pixel offset is: ((refBlockIndex / (240 / 4)) * 240 * 4) + ((refBlockIndex % (240 / 4)) * 4);
                    // division by 60 using shifts, see: http://homepage.divms.uiowa.edu/~jones/bcd/divide.html
                    // calculate refBlockIndex / 15 / 4 with extra precision
                    uint32_t offsetY = ((refBlockIndex >> 3) + refBlockIndex) >> 4;
                    offsetY = (offsetY + refBlockIndex) >> 4;
                    offsetY = (offsetY + refBlockIndex) >> 4;
                    offsetY = (offsetY + refBlockIndex) >> (4 + 2);
                    // calculate refBlockIndex % 60
                    uint32_t offsetX = refBlockIndex - (((offsetY << 4) - offsetY) << 2);
                    // multiply y-offset by stride and add x-offset
                    uint32_t refBlockOffset = offsetY * 240 * 4 + offsetX * 4;
                    auto copySrcPtr = reinterpret_cast<const uint32_t *>(dst + refBlockOffset);
                    auto copyDstPtr = reinterpret_cast<uint32_t *>(blockDst);
                    // copy 4 pixels = 8 bytes from reference to current block
                    copyDstPtr[0] = copySrcPtr[0];
                    copyDstPtr[1] = copySrcPtr[1];
                    // move to next line in source and destination vertically
                    copySrcPtr += LineStride32;
                    copyDstPtr += LineStride32;
                    // copy 4 pixels = 8 bytes from reference to current block
                    copyDstPtr[0] = copySrcPtr[0];
                    copyDstPtr[1] = copySrcPtr[1];
                    // move to next line in source and destination vertically
                    copySrcPtr += LineStride32;
                    copyDstPtr += LineStride32;
                    // copy 4 pixels = 8 bytes from reference to current block
                    copyDstPtr[0] = copySrcPtr[0];
                    copyDstPtr[1] = copySrcPtr[1];
                    // move to next line in source and destination vertically
                    copySrcPtr += LineStride32;
                    copyDstPtr += LineStride32;
                    // copy 4 pixels = 8 bytes from reference to current block
                    copyDstPtr[0] = copySrcPtr[0];
                    copyDstPtr[1] = copySrcPtr[1];
                }
                else
                {
                    // full DXT block. get anchor colors c0 and c1
                    uint32_t c0 = *srcDxtPtr++;
                    uint32_t c1 = *srcDxtPtr++;
                    colors[0] = c0;
                    colors[1] = c1;
                    // calculate intermediate colors c2 and c3
                    uint32_t b = ((c0 & 0x7C00) >> 5) | ((c1 & 0x7C00) >> 10);
                    uint32_t g = (c0 & 0x3E0) | ((c1 & 0x3E0) >> 5);
                    uint32_t r = ((c0 & 0x1F) << 5) | (c1 & 0x1F);
                    *c2c3Ptr = (C2C3table[b] << 10) | (C2C3table[g] << 5) | C2C3table[r];
                    // get pixel color indices and set pixels accordingly
                    uint32_t indices = *reinterpret_cast<const uint32_t *>(srcDxtPtr);
                    srcDxtPtr += 2;
                    // select color by 2 bit index from [c0, c1, c2, c3]
                    blockDst[0] = colors[(indices >> 0) & 0x3];
                    blockDst[1] = colors[(indices >> 2) & 0x3];
                    blockDst[2] = colors[(indices >> 4) & 0x3];
                    blockDst[3] = colors[(indices >> 6) & 0x3];
                    // move to next line in destination vertically
                    blockDst += LineStride16;
                    // select color by 2 bit index from [c0, c1, c2, c3]
                    blockDst[0] = colors[(indices >> 8) & 0x3];
                    blockDst[1] = colors[(indices >> 10) & 0x3];
                    blockDst[2] = colors[(indices >> 12) & 0x3];
                    blockDst[3] = colors[(indices >> 14) & 0x3];
                    // move to next line in destination vertically
                    blockDst += LineStride16;
                    // select color by 2 bit index from [c0, c1, c2, c3]
                    blockDst[0] = colors[(indices >> 16) & 0x3];
                    blockDst[1] = colors[(indices >> 18) & 0x3];
                    blockDst[2] = colors[(indices >> 20) & 0x3];
                    blockDst[3] = colors[(indices >> 22) & 0x3];
                    // move to next line in destination vertically
                    blockDst += LineStride16;
                    // select color by 2 bit index from [c0, c1, c2, c3]
                    blockDst[0] = colors[(indices >> 24) & 0x3];
                    blockDst[1] = colors[(indices >> 26) & 0x3];
                    blockDst[2] = colors[(indices >> 28) & 0x3];
                    blockDst[3] = colors[(indices >> 30) & 0x3];
                }
                // move to next block in destination horizontally
                blockLineDst += BlockStride16;
                flags >>= 2;
                blockIndex++;
            }
            // move to next block line in destination vertically
            currentDst += BlockLineStride16;
        }
    }

}

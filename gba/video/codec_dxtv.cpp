#include "codec_dxtv.h"

#include "memory/memory.h"
#include "print/output.h"

namespace DXTV
{

    // The image is split into 16x16 pixel blocks which can be futher split into 8x8 and 4x4 blocks.
    //
    // Every 16x16 (block size 0) block has one flag:
    // Bit 0: Block handled entirely (0) or block split into 8x8 (1)
    //
    // Every 8x8 block (block size 1) has one flag:
    // Bit 0: Block handled entirely (0) or block split into 4x4 (1)
    //
    // 4x4 block (block size 2) has no flags. If an 8x8 block has been split,
    // 4 block references or DXT blocks will be read from data.
    //
    // Block flags are sent depth first. So if a 16x16 block is split into 4 8x8 children ABCD, its first 8x8 child A is sent first.
    // If that child is split, its 4 4x4 children CDEF are sent first, then child B and so on. This makes sure no flag bits are wasted.
    //
    // DXT and reference blocks differ in their Bit 15. If 0 it is a DXT-block, if 1 a reference block.
    // - DXT blocks store verbatim DXT data (2 * uint16_t RGB555 colors and index data depending on blocks size).
    //   Due to RGB555 having bit 15 == 0, the correct bit is automatically set.
    // - Reference blocks store:
    //   Bit 15: Always 1 (see above)
    //   Bit 14: Current (0) / previous (1) frame Bit
    //   Bit 0-13: Reference index into frame [0,16383]
    //             Range [-16384,-1] is used for references to the current frame.
    //             Range [-8191,8192] is used for references to the previous frame.
    //
    // Frame header format:
    // uint16_t flags;     // e.g. FRAME_IS_PFRAME
    // uint16_t nrOfFlags; // # of blocks > MinDim in frame (determines the size of flags block)
    //
    // then follows data

    constexpr uint16_t FRAME_IS_PFRAME = 0x80; // 0 for B-frames / key frames, 1 for P-frame / inter-frame compression ("predicted frame")
    constexpr uint16_t FRAME_KEEP = 0x40;      // 1 for frames that are considered a direct copy of the previous frame and can be kept

    constexpr uint32_t BLOCK_NO_SPLIT = 0;             // The block is a full block
    constexpr uint32_t BLOCK_IS_SPLIT = 1;             // The block is split into smaller sub-blocks
    constexpr uint32_t BLOCK_IS_REF = (1 << 15);       // The block is a reference into the current or previous frame
    constexpr uint32_t BLOCK_FROM_PREV = (1 << 14);    // The reference block is from from the previous frame
    constexpr uint32_t BLOCK_INDEX_MASK = ~(3U << 14); // Mask to get the block index from the reference info

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

    IWRAM_DATA ALIGN(4) uint16_t DxtColors[4];

    /// @brief Get DXT colors from source, calculate intermediate colors and write to DxtColors array
    /// @return Pointer past DXT colors
    FORCEINLINE auto GetDXTColors(const uint16_t *src16) -> const uint16_t *
    {
        // get anchor colors c0 and c1
        uint32_t c0 = *src16++;
        uint32_t c1 = *src16++;
        DxtColors[0] = c0;
        DxtColors[1] = c1;
        // calculate intermediate colors c2 and c3
        uint32_t b = ((c0 & 0x7C00) >> 5) | ((c1 & 0x7C00) >> 10);
        uint32_t g = (c0 & 0x3E0) | ((c1 & 0x3E0) >> 5);
        uint32_t r = ((c0 & 0x1F) << 5) | (c1 & 0x1F);
        auto DxtC2C3Ptr = reinterpret_cast<uint32_t *>(&DxtColors[2]);
        *DxtC2C3Ptr = (C2C3table[b] << 10) | (C2C3table[g] << 5) | C2C3table[r];
        return src16;
    }

    /// @brief Uncompress 4x4 DXT block
    /// @return Pointer past whole DXT block in src
    template <uint32_t BLOCK_DIM>
    FORCEINLINE auto UncompressBlock(uint16_t *dst16, const uint16_t *src16, uint32_t LineStride16) -> const uint16_t *;

    /// @brief Uncompress 4x4 DXT block
    /// @return Pointer past whole DXT block in src
    template <>
    FORCEINLINE auto UncompressBlock<4>(uint16_t *dst16, const uint16_t *src16, uint32_t LineStride16) -> const uint16_t *
    {
        // full DXT block. get colors
        src16 = GetDXTColors(src16);
        // get pixel color indices and set pixels accordingly
        uint16_t indices = *src16++;
        // select color by 2 bit index from [c0, c1, c2, c3], then move to next line in destination vertically
        dst16[0] = DxtColors[(indices >> 0) & 0x3];
        dst16[1] = DxtColors[(indices >> 2) & 0x3];
        dst16[2] = DxtColors[(indices >> 4) & 0x3];
        dst16[3] = DxtColors[(indices >> 6) & 0x3];
        dst16 += LineStride16;
        dst16[0] = DxtColors[(indices >> 8) & 0x3];
        dst16[1] = DxtColors[(indices >> 10) & 0x3];
        dst16[2] = DxtColors[(indices >> 12) & 0x3];
        dst16[3] = DxtColors[(indices >> 14) & 0x3];
        dst16 += LineStride16;
        indices = *src16++;
        dst16[0] = DxtColors[(indices >> 0) & 0x3];
        dst16[1] = DxtColors[(indices >> 2) & 0x3];
        dst16[2] = DxtColors[(indices >> 4) & 0x3];
        dst16[3] = DxtColors[(indices >> 6) & 0x3];
        dst16 += LineStride16;
        dst16[0] = DxtColors[(indices >> 8) & 0x3];
        dst16[1] = DxtColors[(indices >> 10) & 0x3];
        dst16[2] = DxtColors[(indices >> 12) & 0x3];
        dst16[3] = DxtColors[(indices >> 14) & 0x3];
        return src16;
    }

    /// @brief Uncompress 4x4 DXT block
    /// @return Pointer past whole DXT block in src
    template <>
    FORCEINLINE auto UncompressBlock<8>(uint16_t *dst16, const uint16_t *src16, uint32_t LineStride16) -> const uint16_t *
    {
        // full DXT block. get colors
        src16 = GetDXTColors(src16);
        // get pixel color indices and set pixels accordingly
        for (uint32_t i = 0; i < 4; ++i)
        {
            uint16_t indices = *src16++;
            // select color by 2 bit index from [c0, c1, c2, c3], then move to next line in destination vertically
            dst16[0] = DxtColors[(indices >> 0) & 0x3];
            dst16[1] = DxtColors[(indices >> 2) & 0x3];
            dst16[2] = DxtColors[(indices >> 4) & 0x3];
            dst16[3] = DxtColors[(indices >> 6) & 0x3];
            dst16[4] = DxtColors[(indices >> 8) & 0x3];
            dst16[5] = DxtColors[(indices >> 10) & 0x3];
            dst16[6] = DxtColors[(indices >> 12) & 0x3];
            dst16[7] = DxtColors[(indices >> 14) & 0x3];
            dst16 += LineStride16;
            indices = *src16++;
            dst16[0] = DxtColors[(indices >> 0) & 0x3];
            dst16[1] = DxtColors[(indices >> 2) & 0x3];
            dst16[2] = DxtColors[(indices >> 4) & 0x3];
            dst16[3] = DxtColors[(indices >> 6) & 0x3];
            dst16[4] = DxtColors[(indices >> 8) & 0x3];
            dst16[5] = DxtColors[(indices >> 10) & 0x3];
            dst16[6] = DxtColors[(indices >> 12) & 0x3];
            dst16[7] = DxtColors[(indices >> 14) & 0x3];
            dst16 += LineStride16;
        }
        return src16;
    }

    /// @brief Uncompress 4x4 DXT block
    /// @return Pointer past whole DXT block in src
    template <>
    FORCEINLINE auto UncompressBlock<16>(uint16_t *dst16, const uint16_t *src16, uint32_t LineStride16) -> const uint16_t *
    {
        // full DXT block. get colors
        src16 = GetDXTColors(src16);
        // get pixel color indices and set pixels accordingly
        for (uint32_t i = 0; i < 16; ++i)
        {
            uint16_t indices = *src16++;
            // select color by 2 bit index from [c0, c1, c2, c3], then move to next line in destination vertically
            dst16[0] = DxtColors[(indices >> 0) & 0x3];
            dst16[1] = DxtColors[(indices >> 2) & 0x3];
            dst16[2] = DxtColors[(indices >> 4) & 0x3];
            dst16[3] = DxtColors[(indices >> 6) & 0x3];
            dst16[4] = DxtColors[(indices >> 8) & 0x3];
            dst16[5] = DxtColors[(indices >> 10) & 0x3];
            dst16[6] = DxtColors[(indices >> 12) & 0x3];
            dst16[7] = DxtColors[(indices >> 14) & 0x3];
            indices = *src16++;
            dst16[8] = DxtColors[(indices >> 0) & 0x3];
            dst16[9] = DxtColors[(indices >> 2) & 0x3];
            dst16[10] = DxtColors[(indices >> 4) & 0x3];
            dst16[11] = DxtColors[(indices >> 6) & 0x3];
            dst16[12] = DxtColors[(indices >> 8) & 0x3];
            dst16[13] = DxtColors[(indices >> 10) & 0x3];
            dst16[14] = DxtColors[(indices >> 12) & 0x3];
            dst16[15] = DxtColors[(indices >> 14) & 0x3];
            dst16 += LineStride16;
        }
        return src16;
    }

    template <uint32_t BLOCK_DIM>
    FORCEINLINE void CopyBlock(uint32_t *dst32, const uint32_t *src32, uint32_t LineStride32);

    template <>
    FORCEINLINE void CopyBlock<4>(uint32_t *dst32, const uint32_t *src32, uint32_t LineStride32)
    {
        // copy BLOCK_DIM pixels = 2 * BLOCK_DIM bytes from reference to current block, then move to next line in source and destination vertically
        dst32[0] = src32[0];
        dst32[1] = src32[1];
        src32 += LineStride32;
        dst32 += LineStride32;
        dst32[0] = src32[0];
        dst32[1] = src32[1];
        src32 += LineStride32;
        dst32 += LineStride32;
        dst32[0] = src32[0];
        dst32[1] = src32[1];
        src32 += LineStride32;
        dst32 += LineStride32;
        dst32[0] = src32[0];
        dst32[1] = src32[1];
    }

    template <>
    FORCEINLINE void CopyBlock<8>(uint32_t *dst32, const uint32_t *src32, uint32_t LineStride32)
    {
        for (uint32_t i = 0; i < 4; ++i)
        {
            // copy BLOCK_DIM pixels = 2 * BLOCK_DIM bytes from reference to current block, then move to next line in source and destination vertically
            dst32[0] = src32[0];
            dst32[1] = src32[1];
            dst32[2] = src32[2];
            dst32[3] = src32[3];
            src32 += LineStride32;
            dst32 += LineStride32;
            dst32[0] = src32[0];
            dst32[1] = src32[1];
            dst32[2] = src32[2];
            dst32[3] = src32[3];
            src32 += LineStride32;
            dst32 += LineStride32;
        }
    }

    template <>
    FORCEINLINE void CopyBlock<16>(uint32_t *dst32, const uint32_t *src32, uint32_t LineStride32)
    {
        for (uint32_t i = 0; i < 16; ++i)
        {
            // copy BLOCK_DIM pixels = 2 * BLOCK_DIM bytes from reference to current block, then move to next line in source and destination vertically
            dst32[0] = src32[0];
            dst32[1] = src32[1];
            dst32[2] = src32[2];
            dst32[3] = src32[3];
            dst32[4] = src32[4];
            dst32[5] = src32[5];
            dst32[6] = src32[6];
            dst32[7] = src32[7];
            src32 += LineStride32;
            dst32 += LineStride32;
        }
    }

    template <uint32_t BLOCK_DIM>
    FORCEINLINE auto CalcRefOffset32(uint32_t refBlockIndex) -> uint32_t
    {
        if (refBlockIndex == 0)
        {
            return 0;
        }
        uint32_t offsetY = ((refBlockIndex >> 3) + refBlockIndex) >> 4;
        offsetY = (offsetY + refBlockIndex) >> 4;
        offsetY = (offsetY + refBlockIndex) >> 4;
        if constexpr (BLOCK_DIM == 4)
        {
            // calculate refBlockIndex / 60 = 15 / 4 with extra precision
            offsetY = (offsetY + refBlockIndex) >> (4 + 2);
            // calculate refBlockIndex % 60 = refBlockIndex - (offsetY * 60)
            uint32_t offsetX = refBlockIndex - (((offsetY << 4) - offsetY) << 2);
            // multiply y-offset by stride and add x-offset
            auto refPixelOffset = offsetY * 240 + offsetX;
            return refPixelOffset << 1;
        }
        else if constexpr (BLOCK_DIM == 8)
        {
            // calculate refBlockIndex / 30 = 15 / 2 with extra precision
            offsetY = (offsetY + refBlockIndex) >> (4 + 1);
            // calculate refBlockIndex % 30 = refBlockIndex - (offsetY * 30)
            uint32_t offsetX = refBlockIndex - (((offsetY << 4) - offsetY) << 1);
            // multiply y-offset by stride and add x-offset
            auto refPixelOffset = offsetY * 240 + offsetX;
            return refPixelOffset << 2;
        }
        else if constexpr (BLOCK_DIM == 16)
        {
            // calculate refBlockIndex / 15 with extra precision
            offsetY = (offsetY + refBlockIndex) >> 4;
            // calculate refBlockIndex % 15 = refBlockIndex - (offsetY * 15)
            uint32_t offsetX = refBlockIndex - ((offsetY << 4) - offsetY);
            // multiply y-offset by stride and add x-offset
            auto refPixelOffset = offsetY * 240 + offsetX;
            return refPixelOffset << 3;
        }
    }

    /// @brief Uncompress one BLOCK_DIM*BLOCK_DIM block
    /// @return Pointer past whole block in src
    template <uint32_t BLOCK_DIM>
    FORCEINLINE auto DecodeBlock240(uint32_t *block32, const uint16_t *src16, uint32_t *curr32, const uint32_t *prev32, uint32_t LineStride32) -> const uint16_t *
    {
        // check if block is DXT or reference
        auto blockFlags = static_cast<uint32_t>(*src16);
        if ((blockFlags & BLOCK_IS_REF) != 0)
        {
            const bool isFromPrev = (blockFlags & BLOCK_FROM_PREV) != 0;
            auto refSrc32 = isFromPrev ? prev32 : curr32;
            uint32_t refBlockIndex = blockFlags & BLOCK_INDEX_MASK;
            CopyBlock<BLOCK_DIM>(block32, refSrc32 + CalcRefOffset32<BLOCK_DIM>(refBlockIndex), LineStride32);
            // FillBlock<BLOCK_DIM>(block32, isFromPrev ? 0x03E003E0 : 0x03FF03FF, LineStride32);
            return src16 + 1;
        }
        else
        {
            return UncompressBlock<BLOCK_DIM>(reinterpret_cast<uint16_t *>(block32), src16, LineStride32 << 1);
        }
    }

    template <>
    IWRAM_FUNC void UnCompWrite16bit<240>(uint32_t *dst, const uint32_t *src, const uint32_t *prevSrc, uint32_t width, uint32_t height)
    {
        constexpr uint32_t LineStride32 = 120;                                                                               // stride to next line in dst in words / 2 pixels
        constexpr uint32_t Block4LineStride32 = LineStride32 * 4;                                                            // vertical stride to next 4x4 block in dst (4 lines)
        constexpr uint32_t Block8LineStride32 = LineStride32 * 8;                                                            // vertical stride to next 8x8 block in dst (8 lines)
        constexpr uint32_t Block16LineStride32 = LineStride32 * 16;                                                          // vertical stride to next 16x16 block in dst (16 lines)
        constexpr uint32_t Block4Stride32 = 2;                                                                               // horizontal stride to next 4x4 block (4 * 2 bytes)
        constexpr uint32_t Block8Stride32 = 4;                                                                               // horizontal stride to next 8x8 block (8 * 2 bytes)
        constexpr uint32_t Block16Stride32 = 8;                                                                              // horizontal stride to next 16x6 block (16 * 2 bytes)
        constexpr uint32_t Block4Offsets32[] = {0, Block4Stride32, Block4LineStride32, Block4LineStride32 + Block4Stride32}; // block pixel offsets from parent 8x8 block
        constexpr uint32_t Block8Offsets32[] = {0, Block8Stride32, Block8LineStride32, Block8LineStride32 + Block8Stride32}; // block pixel offsets from parent 16x16 block
        auto src16 = reinterpret_cast<const uint16_t *>(src);
        // copy frame header
        uint32_t headerFlags = *src16++;
        // check if we want to keep this duplicate frame
        const bool keepFrame = (headerFlags & FRAME_KEEP) != 0;
        if (keepFrame)
        {
            // Debug::printf("Duplicate frame");
            return;
        }
        // check if this frame is a key frame
        /*const bool isKeyFrame = (headerFlags & FRAME_IS_PFRAME) == 0;
        if (isKeyFrame)
        {
            Debug::printf("Key frame");
        }*/
        // set up some variables
        uint32_t splitFlags = 0;                                                                     // Block split bit flags for blocks
        uint32_t splitFlagsAvailable = 0;                                                            // How many bits we have left in flags
        uint32_t nrOfFlags = *src16++;                                                               // # of 16x16 and 8x8 block flags stored
        auto srcSplitFlagPtr = src16;                                                                // flags that define encoding state of blocks
        const uint32_t nrOfFlagHwords = ((static_cast<uint32_t>(nrOfFlags) + 31) & 0xFFFFFFE0) >> 4; // round # of flags up to multiple of 32 and divide by 16
        auto srcDataPtr = srcSplitFlagPtr + nrOfFlagHwords;                                          // DXT block data. offset is nrOfRefBlocks bytes
        auto currentDst32 = dst;
        for (uint32_t blockY = 0; blockY < height / 16; blockY++)
        {
            auto block16Dst32 = currentDst32;
            for (uint32_t blockX = 0; blockX < width / 16; blockX++)
            {
                // make sure we have enough flags left for one 16x16 and 4 8x8 blocks
                if (splitFlagsAvailable < 5)
                {
                    splitFlags |= (static_cast<uint32_t>(*srcSplitFlagPtr++) << splitFlagsAvailable);
                    splitFlagsAvailable += 16;
                }
                // check if 16x16 block is split
                const bool isSplit16 = (splitFlags & BLOCK_IS_SPLIT) != 0;
                splitFlags >>= 1;
                splitFlagsAvailable--;
                if (isSplit16)
                {
                    // decode 4 8x8 blocks
                    for (uint32_t i8 = 0; i8 < 4; ++i8)
                    {
                        auto block8Dst32 = block16Dst32 + Block8Offsets32[i8];
                        // check if 8x8 block is split again
                        const bool isSplit8 = (splitFlags & BLOCK_IS_SPLIT) != 0;
                        splitFlags >>= 1;
                        splitFlagsAvailable--;
                        if (isSplit8)
                        {
                            // decode 4 4x4 blocks
                            for (uint32_t i4 = 0; i4 < 4; ++i4)
                            {
                                srcDataPtr = DecodeBlock240<4>(block8Dst32 + Block4Offsets32[i4], srcDataPtr, dst, prevSrc, LineStride32);
                            }
                        }
                        else
                        {
                            // decode single 8x8 block
                            srcDataPtr = DecodeBlock240<8>(block8Dst32, srcDataPtr, dst, prevSrc, LineStride32);
                        }
                    }
                }
                else
                {
                    // decode single 16x16 block
                    srcDataPtr = DecodeBlock240<16>(block16Dst32, srcDataPtr, dst, prevSrc, LineStride32);
                }
                // move to next 16x16 block in destination horizontally
                block16Dst32 += Block16Stride32;
            }
            // move to next 16x16 block line in destination vertically
            currentDst32 += Block16LineStride32;
        }
    }
}

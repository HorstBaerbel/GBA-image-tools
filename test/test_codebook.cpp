#include "testmacros.h"

#include "color/xrgb8888.h"
#include "video_codec/codebook.h"

#include <array>
#include <cstdint>
#include <vector>

static std::vector<uint32_t> Pixels24x8 = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7};

static std::vector<uint32_t> Pixels16x16 = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};

static std::array<std::vector<uint32_t>, 4> Pixels8x8 = {{{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                                           0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                                           0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
                                                           0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                                                           0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
                                                           0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
                                                           0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
                                                           0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77},
                                                          {0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                                                           0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
                                                           0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
                                                           0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
                                                           0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
                                                           0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
                                                           0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
                                                           0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f},
                                                          {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
                                                           0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
                                                           0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
                                                           0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
                                                           0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
                                                           0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
                                                           0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
                                                           0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7},
                                                          {0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
                                                           0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
                                                           0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
                                                           0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
                                                           0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
                                                           0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
                                                           0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
                                                           0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff}}};

static std::array<std::vector<uint32_t>, 16> Pixels4x4 = {{{0x00, 0x01, 0x02, 0x03,
                                                            0x10, 0x11, 0x12, 0x13,
                                                            0x20, 0x21, 0x22, 0x23,
                                                            0x30, 0x31, 0x32, 0x33},
                                                           {0x04, 0x05, 0x06, 0x07,
                                                            0x14, 0x15, 0x16, 0x17,
                                                            0x24, 0x25, 0x26, 0x27,
                                                            0x34, 0x35, 0x36, 0x37},
                                                           {0x40, 0x41, 0x42, 0x43,
                                                            0x50, 0x51, 0x52, 0x53,
                                                            0x60, 0x61, 0x62, 0x63,
                                                            0x70, 0x71, 0x72, 0x73},
                                                           {0x44, 0x45, 0x46, 0x47,
                                                            0x54, 0x55, 0x56, 0x57,
                                                            0x64, 0x65, 0x66, 0x67,
                                                            0x74, 0x75, 0x76, 0x77},
                                                           {0x08, 0x09, 0x0a, 0x0b,
                                                            0x18, 0x19, 0x1a, 0x1b,
                                                            0x28, 0x29, 0x2a, 0x2b,
                                                            0x38, 0x39, 0x3a, 0x3b},
                                                           {0x0c, 0x0d, 0x0e, 0x0f,
                                                            0x1c, 0x1d, 0x1e, 0x1f,
                                                            0x2c, 0x2d, 0x2e, 0x2f,
                                                            0x3c, 0x3d, 0x3e, 0x3f},
                                                           {0x48, 0x49, 0x4a, 0x4b,
                                                            0x58, 0x59, 0x5a, 0x5b,
                                                            0x68, 0x69, 0x6a, 0x6b,
                                                            0x78, 0x79, 0x7a, 0x7b},
                                                           {0x4c, 0x4d, 0x4e, 0x4f,
                                                            0x5c, 0x5d, 0x5e, 0x5f,
                                                            0x6c, 0x6d, 0x6e, 0x6f,
                                                            0x7c, 0x7d, 0x7e, 0x7f},
                                                           {0x80, 0x81, 0x82, 0x83,
                                                            0x90, 0x91, 0x92, 0x93,
                                                            0xa0, 0xa1, 0xa2, 0xa3,
                                                            0xb0, 0xb1, 0xb2, 0xb3},
                                                           {0x84, 0x85, 0x86, 0x87,
                                                            0x94, 0x95, 0x96, 0x97,
                                                            0xa4, 0xa5, 0xa6, 0xa7,
                                                            0xb4, 0xb5, 0xb6, 0xb7},
                                                           {0xc0, 0xc1, 0xc2, 0xc3,
                                                            0xd0, 0xd1, 0xd2, 0xd3,
                                                            0xe0, 0xe1, 0xe2, 0xe3,
                                                            0xf0, 0xf1, 0xf2, 0xf3},
                                                           {0xc4, 0xc5, 0xc6, 0xc7,
                                                            0xd4, 0xd5, 0xd6, 0xd7,
                                                            0xe4, 0xe5, 0xe6, 0xe7,
                                                            0xf4, 0xf5, 0xf6, 0xf7},
                                                           {0x88, 0x89, 0x8a, 0x8b,
                                                            0x98, 0x99, 0x9a, 0x9b,
                                                            0xa8, 0xa9, 0xaa, 0xab,
                                                            0xb8, 0xb9, 0xba, 0xbb},
                                                           {0x8c, 0x8d, 0x8e, 0x8f,
                                                            0x9c, 0x9d, 0x9e, 0x9f,
                                                            0xac, 0xad, 0xae, 0xaf,
                                                            0xbc, 0xbd, 0xbe, 0xbf},
                                                           {0xc8, 0xc9, 0xca, 0xcb,
                                                            0xd8, 0xd9, 0xda, 0xdb,
                                                            0xe8, 0xe9, 0xea, 0xeb,
                                                            0xf8, 0xf9, 0xfa, 0xfb},
                                                           {0xcc, 0xcd, 0xce, 0xcf,
                                                            0xdc, 0xdd, 0xde, 0xdf,
                                                            0xec, 0xed, 0xee, 0xef,
                                                            0xfc, 0xfd, 0xfe, 0xff}}};

TEST_SUITE("Code book")

TEST_CASE("Bad initialization throws")
{
    CodeBook<uint32_t, 16, 4> badsize16;
    CATCH_REQUIRE_THROWS(badsize16 = CodeBook<uint32_t, 16, 4>(Pixels16x16, 15, 16));
    CATCH_REQUIRE_THROWS(badsize16 = CodeBook<uint32_t, 16, 4>(Pixels16x16, 16, 9));
    CATCH_REQUIRE_THROWS(badsize16 = CodeBook<uint32_t, 16, 4>(Pixels16x16, 32, 8));
    CATCH_REQUIRE_THROWS(badsize16 = CodeBook<uint32_t, 16, 4>(Pixels16x16, 8, 32));
}

TEST_CASE("Initialization")
{
    CodeBook<uint32_t, 16, 4> cb16(Pixels16x16, 16, 16);
    CATCH_CHECK(CodeBook<uint32_t, 16, 4>::BlockLevels == 3);
    CATCH_CHECK(CodeBook<uint32_t, 16, 4>::BlockMaxDim == 16);
    CATCH_CHECK(CodeBook<uint32_t, 16, 4>::BlockMinDim == 4);
    CATCH_CHECK(CodeBook<uint32_t, 16, 4>::HasBlockLevel2 == true);
    CATCH_CHECK(cb16.width() == 16);
    CATCH_CHECK(cb16.height() == 16);
    CATCH_CHECK(cb16.blockWidth() == 1);
    CATCH_CHECK(cb16.blockWidth<16>() == 1);
    CATCH_CHECK(cb16.blockWidth<8>() == 2);
    CATCH_CHECK(cb16.blockWidth<4>() == 4);
    CATCH_CHECK(cb16.blockHeight() == 1);
    CATCH_CHECK(cb16.blockHeight<16>() == 1);
    CATCH_CHECK(cb16.blockHeight<8>() == 2);
    CATCH_CHECK(cb16.blockHeight<4>() == 4);
    CATCH_CHECK(cb16.size() == 1);
    CATCH_CHECK(cb16.pixels() == Pixels16x16);
    CodeBook<uint32_t, 8, 4> cb8(Pixels24x8, 24, 8);
    CATCH_CHECK(CodeBook<uint32_t, 8, 4>::BlockLevels == 2);
    CATCH_CHECK(CodeBook<uint32_t, 8, 4>::BlockMaxDim == 8);
    CATCH_CHECK(CodeBook<uint32_t, 8, 4>::BlockMinDim == 4);
    CATCH_CHECK(CodeBook<uint32_t, 8, 4>::HasBlockLevel2 == false);
    CATCH_CHECK(cb8.width() == 24);
    CATCH_CHECK(cb8.height() == 8);
    CATCH_CHECK(cb8.blockWidth() == 3);
    CATCH_CHECK(cb8.blockHeight() == 1);
    CATCH_CHECK(cb8.size() == 3);
    CATCH_CHECK(cb8.pixels() == Pixels24x8);
}

TEST_CASE("Block access 16x16")
{
    CodeBook<uint32_t, 16, 4> cb16(Pixels16x16, 16, 16);
    // 16x16 blocks
    const auto &cb16_0 = cb16.block(0);
    // positions
    CATCH_CHECK(cb16_0.x() == 0);
    CATCH_CHECK(cb16_0.y() == 0);
    // indices
    CATCH_CHECK(cb16_0.index() == 0);
    // pixels
    CATCH_CHECK(cb16_0.pixels() == Pixels16x16);
    // 8x8 blocks
    const auto &cb16_10 = cb16_0.block(0);
    const auto &cb16_11 = cb16_0.block(1);
    const auto &cb16_12 = cb16_0.block(2);
    const auto &cb16_13 = cb16_0.block(3);
    // positions
    CATCH_CHECK(cb16_10.x() == 0);
    CATCH_CHECK(cb16_10.y() == 0);
    CATCH_CHECK(cb16_11.x() == 8);
    CATCH_CHECK(cb16_11.y() == 0);
    CATCH_CHECK(cb16_12.x() == 0);
    CATCH_CHECK(cb16_12.y() == 8);
    CATCH_CHECK(cb16_13.x() == 8);
    CATCH_CHECK(cb16_13.y() == 8);
    // indices
    CATCH_CHECK(cb16_10.index() == 0);
    CATCH_CHECK(cb16_11.index() == 1);
    CATCH_CHECK(cb16_12.index() == 2);
    CATCH_CHECK(cb16_13.index() == 3);
    // pixels
    CATCH_CHECK(cb16_10.pixels() == Pixels8x8[0]);
    CATCH_CHECK(cb16_11.pixels() == Pixels8x8[1]);
    CATCH_CHECK(cb16_12.pixels() == Pixels8x8[2]);
    CATCH_CHECK(cb16_13.pixels() == Pixels8x8[3]);
    // 4x4 blocks
    const auto &cb16_20 = cb16_10.block(0);
    const auto &cb16_21 = cb16_10.block(1);
    const auto &cb16_22 = cb16_10.block(2);
    const auto &cb16_23 = cb16_10.block(3);
    const auto &cb16_24 = cb16_11.block(0);
    const auto &cb16_25 = cb16_11.block(1);
    const auto &cb16_26 = cb16_11.block(2);
    const auto &cb16_27 = cb16_11.block(3);
    const auto &cb16_28 = cb16_12.block(0);
    const auto &cb16_29 = cb16_12.block(1);
    const auto &cb16_210 = cb16_12.block(2);
    const auto &cb16_211 = cb16_12.block(3);
    const auto &cb16_212 = cb16_13.block(0);
    const auto &cb16_213 = cb16_13.block(1);
    const auto &cb16_214 = cb16_13.block(2);
    const auto &cb16_215 = cb16_13.block(3);
    // positions
    CATCH_CHECK(cb16_20.x() == 0);
    CATCH_CHECK(cb16_20.y() == 0);
    CATCH_CHECK(cb16_21.x() == 4);
    CATCH_CHECK(cb16_21.y() == 0);
    CATCH_CHECK(cb16_22.x() == 0);
    CATCH_CHECK(cb16_22.y() == 4);
    CATCH_CHECK(cb16_23.x() == 4);
    CATCH_CHECK(cb16_23.y() == 4);
    CATCH_CHECK(cb16_24.x() == 8);
    CATCH_CHECK(cb16_24.y() == 0);
    CATCH_CHECK(cb16_25.x() == 12);
    CATCH_CHECK(cb16_25.y() == 0);
    CATCH_CHECK(cb16_26.x() == 8);
    CATCH_CHECK(cb16_26.y() == 4);
    CATCH_CHECK(cb16_27.x() == 12);
    CATCH_CHECK(cb16_27.y() == 4);
    CATCH_CHECK(cb16_28.x() == 0);
    CATCH_CHECK(cb16_28.y() == 8);
    CATCH_CHECK(cb16_29.x() == 4);
    CATCH_CHECK(cb16_29.y() == 8);
    CATCH_CHECK(cb16_210.x() == 0);
    CATCH_CHECK(cb16_210.y() == 12);
    CATCH_CHECK(cb16_211.x() == 4);
    CATCH_CHECK(cb16_211.y() == 12);
    CATCH_CHECK(cb16_212.x() == 8);
    CATCH_CHECK(cb16_212.y() == 8);
    CATCH_CHECK(cb16_213.x() == 12);
    CATCH_CHECK(cb16_213.y() == 8);
    CATCH_CHECK(cb16_214.x() == 8);
    CATCH_CHECK(cb16_214.y() == 12);
    CATCH_CHECK(cb16_215.x() == 12);
    CATCH_CHECK(cb16_215.y() == 12);
    // indices
    CATCH_CHECK(cb16_20.index() == 0);
    CATCH_CHECK(cb16_21.index() == 1);
    CATCH_CHECK(cb16_22.index() == 4);
    CATCH_CHECK(cb16_23.index() == 5);
    CATCH_CHECK(cb16_24.index() == 2);
    CATCH_CHECK(cb16_25.index() == 3);
    CATCH_CHECK(cb16_26.index() == 6);
    CATCH_CHECK(cb16_27.index() == 7);
    CATCH_CHECK(cb16_28.index() == 8);
    CATCH_CHECK(cb16_29.index() == 9);
    CATCH_CHECK(cb16_210.index() == 12);
    CATCH_CHECK(cb16_211.index() == 13);
    CATCH_CHECK(cb16_212.index() == 10);
    CATCH_CHECK(cb16_213.index() == 11);
    CATCH_CHECK(cb16_214.index() == 14);
    CATCH_CHECK(cb16_215.index() == 15);
    // pixels
    CATCH_CHECK(cb16_20.pixels() == Pixels4x4[0]);
    CATCH_CHECK(cb16_21.pixels() == Pixels4x4[1]);
    CATCH_CHECK(cb16_22.pixels() == Pixels4x4[2]);
    CATCH_CHECK(cb16_23.pixels() == Pixels4x4[3]);
    CATCH_CHECK(cb16_24.pixels() == Pixels4x4[4]);
    CATCH_CHECK(cb16_25.pixels() == Pixels4x4[5]);
    CATCH_CHECK(cb16_26.pixels() == Pixels4x4[6]);
    CATCH_CHECK(cb16_27.pixels() == Pixels4x4[7]);
    CATCH_CHECK(cb16_28.pixels() == Pixels4x4[8]);
    CATCH_CHECK(cb16_29.pixels() == Pixels4x4[9]);
    CATCH_CHECK(cb16_210.pixels() == Pixels4x4[10]);
    CATCH_CHECK(cb16_211.pixels() == Pixels4x4[11]);
    CATCH_CHECK(cb16_212.pixels() == Pixels4x4[12]);
    CATCH_CHECK(cb16_213.pixels() == Pixels4x4[13]);
    CATCH_CHECK(cb16_214.pixels() == Pixels4x4[14]);
    CATCH_CHECK(cb16_215.pixels() == Pixels4x4[15]);
}

TEST_CASE("Block access 24x8")
{
    CodeBook<uint32_t, 8, 4> cb8(Pixels24x8, 24, 8);
    // 8x8 blocks
    const auto &cb8_0 = cb8.block(0);
    const auto &cb8_1 = cb8.block(1);
    const auto &cb8_2 = cb8.block(2);
    // positions
    CATCH_CHECK(cb8_0.x() == 0);
    CATCH_CHECK(cb8_0.y() == 0);
    CATCH_CHECK(cb8_1.x() == 8);
    CATCH_CHECK(cb8_1.y() == 0);
    CATCH_CHECK(cb8_2.x() == 16);
    CATCH_CHECK(cb8_2.y() == 0);
    // indices
    CATCH_CHECK(cb8_0.index() == 0);
    CATCH_CHECK(cb8_1.index() == 1);
    CATCH_CHECK(cb8_2.index() == 2);
    // pixels
    CATCH_CHECK(cb8_0.pixels() == Pixels8x8[0]);
    CATCH_CHECK(cb8_1.pixels() == Pixels8x8[1]);
    CATCH_CHECK(cb8_2.pixels() == Pixels8x8[2]);
    // 4x4 blocks
    const auto &cb8_10 = cb8_0.block(0);
    const auto &cb8_11 = cb8_0.block(1);
    const auto &cb8_12 = cb8_0.block(2);
    const auto &cb8_13 = cb8_0.block(3);
    const auto &cb8_14 = cb8_1.block(0);
    const auto &cb8_15 = cb8_1.block(1);
    const auto &cb8_16 = cb8_1.block(2);
    const auto &cb8_17 = cb8_1.block(3);
    const auto &cb8_18 = cb8_2.block(0);
    const auto &cb8_19 = cb8_2.block(1);
    const auto &cb8_110 = cb8_2.block(2);
    const auto &cb8_111 = cb8_2.block(3);
    // positions
    CATCH_CHECK(cb8_10.x() == 0);
    CATCH_CHECK(cb8_10.y() == 0);
    CATCH_CHECK(cb8_11.x() == 4);
    CATCH_CHECK(cb8_11.y() == 0);
    CATCH_CHECK(cb8_12.x() == 0);
    CATCH_CHECK(cb8_12.y() == 4);
    CATCH_CHECK(cb8_13.x() == 4);
    CATCH_CHECK(cb8_13.y() == 4);
    CATCH_CHECK(cb8_14.x() == 8);
    CATCH_CHECK(cb8_14.y() == 0);
    CATCH_CHECK(cb8_15.x() == 12);
    CATCH_CHECK(cb8_15.y() == 0);
    CATCH_CHECK(cb8_16.x() == 8);
    CATCH_CHECK(cb8_16.y() == 4);
    CATCH_CHECK(cb8_17.x() == 12);
    CATCH_CHECK(cb8_17.y() == 4);
    CATCH_CHECK(cb8_18.x() == 16);
    CATCH_CHECK(cb8_18.y() == 0);
    CATCH_CHECK(cb8_19.x() == 20);
    CATCH_CHECK(cb8_19.y() == 0);
    CATCH_CHECK(cb8_110.x() == 16);
    CATCH_CHECK(cb8_110.y() == 4);
    CATCH_CHECK(cb8_111.x() == 20);
    CATCH_CHECK(cb8_111.y() == 4);
    // indices
    CATCH_CHECK(cb8_10.index() == 0);
    CATCH_CHECK(cb8_11.index() == 1);
    CATCH_CHECK(cb8_12.index() == 6);
    CATCH_CHECK(cb8_13.index() == 7);
    CATCH_CHECK(cb8_14.index() == 2);
    CATCH_CHECK(cb8_15.index() == 3);
    CATCH_CHECK(cb8_16.index() == 8);
    CATCH_CHECK(cb8_17.index() == 9);
    CATCH_CHECK(cb8_18.index() == 4);
    CATCH_CHECK(cb8_19.index() == 5);
    CATCH_CHECK(cb8_110.index() == 10);
    CATCH_CHECK(cb8_111.index() == 11);
    // pixels
    CATCH_CHECK(cb8_10.pixels() == Pixels4x4[0]);
    CATCH_CHECK(cb8_11.pixels() == Pixels4x4[1]);
    CATCH_CHECK(cb8_12.pixels() == Pixels4x4[2]);
    CATCH_CHECK(cb8_13.pixels() == Pixels4x4[3]);
    CATCH_CHECK(cb8_14.pixels() == Pixels4x4[4]);
    CATCH_CHECK(cb8_15.pixels() == Pixels4x4[5]);
    CATCH_CHECK(cb8_16.pixels() == Pixels4x4[6]);
    CATCH_CHECK(cb8_17.pixels() == Pixels4x4[7]);
    CATCH_CHECK(cb8_18.pixels() == Pixels4x4[8]);
    CATCH_CHECK(cb8_19.pixels() == Pixels4x4[9]);
    CATCH_CHECK(cb8_110.pixels() == Pixels4x4[10]);
    CATCH_CHECK(cb8_111.pixels() == Pixels4x4[11]);
}

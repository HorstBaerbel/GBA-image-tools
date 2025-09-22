#pragma once

// DXTV encoding:
//
// Header:
//
// uint16_t frameFlags -> General frame flags, e.g. FRAME_KEEP
// uint16_t dummy -> empty atm
//
// Image data:
//
// The image is split into 8x8 pixel blocks (DXTV_CONSTANTS::BLOCK_MAX_DIM) which can be futher split into 4x4 blocks.
//
// Every 8x8 block (block size 0) has one flag:
// Bit 0: Block handled entirely (0) or block split into 4x4 (1)
// These bits are sent in the bitstream for each horizontal 8x8 line in intervals of 16 blocks
// A 240 pixel image stream will send:
// - 16 bits at the start of the bitstream
// - Another 16 bits after 16 encoded blocks (with 2 unused bits)
//
// A 4x4 block (block size 1) has no extra flags. If an 8x8 block has been split,
// 4 motion-compensated or 4 DXT blocks will be read from data.
//
// Blocks are sent row-wise. So if a 8x8 block is split into 4 4x4 children ABCD,
// its first 4x4 child A is sent first then child B and so on. The layout in the image is:
// A B
// C D
//
// 8x8 and 4x4 DXT and motion-compensated blocks differ in their highest Bit:
//
// - If 0 it is a DXT-block with a size of 8 or 20 Bytes:
//   DXT blocks store verbatim DXT data (2 * uint16_t RGB555 colors and index data depending on blocks size)
//   So either 2 * 2 + 16 * 2 / 8 = 8 Bytes (4x4 block) or 2 * 2 + 64 * 2 / 8 = 20 Bytes (8x8 block)
//
// - If 1 it is a motion-compensated block with a size of 2 Bytes:
//   Bit 15: Always 1 (see above)
//   Bit 14: Block is reference to current (0) or previous (1) frame
//   Bit 13-11: Currently unused
//   Bit 10-5: y pixel motion of referenced block [-15,16] from top-left corner
//   Bit  4-0: x pixel motion of referenced block [-15,16] from top-left corner

#ifndef __ASSEMBLER__
// DXTV frame format constants for including in C++ files
#include <cstdint>

namespace Video::DxtvConstants
{
    static constexpr uint8_t FRAME_KEEP = 0x40;                                           // 1 for frames that are considered a direct copy of the previous frame and can be kept
    static constexpr uint32_t BLOCK_MAX_DIM = 8;                                          // Maximum block size is 8x8 pixels
    static constexpr bool BLOCK_NO_SPLIT = false;                                         // The block is a full block
    static constexpr bool BLOCK_IS_SPLIT = true;                                          // The block is split into smaller sub-blocks
    static constexpr uint16_t BLOCK_IS_DXT = 0;                                           // The block is a verbatim DXT block
    static constexpr uint16_t BLOCK_IS_REF = (1 << 15);                                   // The block is a motion-compensated block from the current or previous frame
    static constexpr uint16_t BLOCK_FROM_CURR = (0 << 14);                                // The reference block is from from the current frame
    static constexpr uint16_t BLOCK_FROM_PREV = (1 << 14);                                // The reference block is from from the previous frame
    static constexpr uint32_t BLOCK_MOTION_BITS = 5;                                      // Bits available for pixel motion
    static constexpr uint16_t BLOCK_MOTION_MASK = (uint16_t(1) << BLOCK_MOTION_BITS) - 1; // Block x pixel motion mask
    static constexpr uint16_t BLOCK_MOTION_Y_SHIFT = BLOCK_MOTION_BITS;                   // Block y pixel motion shift
    static constexpr uint32_t BLOCK_HALF_RANGE = (1 << BLOCK_MOTION_BITS) / 2 - 1;        // Half range of pixel motion values [-15,15] from top-left corner
}

#else
// DXTV frame format constants for including in assembly files
#define DXTV_CONSTANTS_FRAME_KEEP 0x40                                                    // 1 for frames that are considered a direct copy of the previous frame and can be kept
#define DXTV_CONSTANTS_BLOCK_MAX_DIM 8                                                    // Maximum block size is 8x8 pixels
#define DXTV_CONSTANTS_BLOCK_IS_REF (1 << 15)                                             // The block is a motion-compensated block from the current or previous frame
#define DXTV_CONSTANTS_BLOCK_FROM_PREV (1 << 14)                                          // The reference block is from from the previous frame
#define DXTV_CONSTANTS_BLOCK_MOTION_BITS 5                                                // Bits available for pixel motion
#define DXTV_CONSTANTS_BLOCK_MOTION_MASK ((1 << DXTV_CONSTANTS_BLOCK_MOTION_BITS) - 1)    // Block x pixel motion mask
#define DXTV_CONSTANTS_BLOCK_MOTION_Y_SHIFT DXTV_CONSTANTS_BLOCK_MOTION_BITS              // Block y pixel motion shift
#define DXTV_CONSTANTS_BLOCK_HALF_RANGE ((1 << DXTV_CONSTANTS_BLOCK_MOTION_BITS) / 2 - 1) // Half range of pixel motion values [-15,15] from top-left corner

#endif
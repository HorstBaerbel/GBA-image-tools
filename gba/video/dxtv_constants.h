#pragma once

#ifndef __ASSEMBLER__
// Constants for including in C++ files
#include <cstdint>

static constexpr uint16_t FRAME_KEEP = 0x40;                                          // 1 for frames that are considered a direct copy of the previous frame and can be kept
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

#else
// Constants for including in assembly files
#define FRAME_KEEP 0x40                                     // 1 for frames that are considered a direct copy of the previous frame and can be kept
#define BLOCK_MAX_DIM 8                                     // Maximum block size is 8x8 pixels
#define BLOCK_IS_REF (1 << 15)                              // The block is a motion-compensated block from the current or previous frame
#define BLOCK_FROM_PREV (1 << 14)                           // The reference block is from from the previous frame
#define BLOCK_MOTION_BITS 5                                 // Bits available for pixel motion
#define BLOCK_MOTION_MASK ((1 << BLOCK_MOTION_BITS) - 1)    // Block x pixel motion mask
#define BLOCK_MOTION_Y_SHIFT BLOCK_MOTION_BITS              // Block y pixel motion shift
#define BLOCK_HALF_RANGE ((1 << BLOCK_MOTION_BITS) / 2 - 1) // Half range of pixel motion values [-15,15] from top-left corner

#endif
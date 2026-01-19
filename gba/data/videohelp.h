// Converted with img2h --paletted=16 --outformat=bgr555 --tiles --sprites=64,32 --prune=4 data/videohelp.png data/videohelp
// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))

// Data is bitmap sprites tiles, pixel format: Paletted 4-bit, color map format: XBGR1555

#pragma once
#include <stdint.h>

#define VIDEOHELP_WIDTH 64 // width of sprites/tiles in pixels
#define VIDEOHELP_HEIGHT 32 // height of sprites/tiles in pixels
#define VIDEOHELP_BYTES_PER_TILE 1024 // bytes for one complete sprite/tile
#define VIDEOHELP_DATA_SIZE 2304 // size of sprite/tile data in 4 byte units
#define VIDEOHELP_NR_OF_TILES 9 // # of sprites/tiles in data
extern const uint32_t VIDEOHELP_DATA[VIDEOHELP_DATA_SIZE];
#define VIDEOHELP_PALETTE_LENGTH 7 // # of palette entries per palette
#define VIDEOHELP_PALETTE_SIZE 16 // size of palette data
extern const uint8_t VIDEOHELP_PALETTE[VIDEOHELP_PALETTE_SIZE];


// Converted with img2h
// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))

#pragma once
#include <stdint.h>

#define FONT_8X8_WIDTH 8 // width of sprites/tiles in pixels
#define FONT_8X8_HEIGHT 8 // height of sprites/tiles in pixels
#define FONT_8X8_BYTES_PER_TILE 32 // bytes for one complete sprite/tile
#define FONT_8X8_DATA_SIZE 768 // size of sprite/tile data in DWORDs
#define FONT_8X8_NR_OF_TILES 96 // # of sprites/tiles in data
extern const uint32_t FONT_8X8_DATA[FONT_8X8_DATA_SIZE];
#define FONT_8X8_PALETTE_LENGTH 2 // # of palette entries per palette
#define FONT_8X8_PALETTE_SIZE 2 // size of palette data in WORDs
extern const uint16_t FONT_8X8_PALETTE[FONT_8X8_PALETTE_SIZE];


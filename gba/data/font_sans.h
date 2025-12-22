// Converted with img2h --blackwhite=0.5 --outformat=bgr555 data/font_sans.png data/font_sans
// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))

// Data is bitmap tiles, pixel format: Paletted 8-bit, color map format: XBGR1555

#pragma once
#include <stdint.h>

#define FONT_SANS_WIDTH 557 // width of image in pixels
#define FONT_SANS_HEIGHT 8 // height of image in pixels
#define FONT_SANS_BYTES_PER_IMAGE 4456 // bytes for one complete image
#define FONT_SANS_DATA_SIZE 1114 // size of image data in 4 byte units
extern const uint32_t FONT_SANS_DATA[FONT_SANS_DATA_SIZE];
#define FONT_SANS_PALETTE_LENGTH 2 // # of palette entries per palette
#define FONT_SANS_PALETTE_SIZE 4 // size of palette data
extern const uint8_t FONT_SANS_PALETTE[FONT_SANS_PALETTE_SIZE];


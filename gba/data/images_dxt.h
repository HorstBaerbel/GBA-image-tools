// Converted with img2h --truecolor=rgb888 --outformat=bgr555 --dxt --lz10 --vram=true ../data/images/240x160/artificial_240x160.png ../data/images/240x160/BigBuckBunny_282_240x160.png ../data/images/240x160/BigBuckBunny_361_240x160.png ../data/images/240x160/BigBuckBunny_40_240x160.png ../data/images/240x160/BigBuckBunny_648_240x160.png ../data/images/240x160/BigBuckBunny_664_240x160.png ../data/images/240x160/flower_foveon_240x160.png ../data/images/240x160/gradient_240x160.png ../data/images/240x160/nightshot_iso_100_240x160.png ../data/images/240x160/squish_240x160.png ../data/images/240x160/TearsOfSteel_1200_240x160.png ../data/images/240x160/TearsOfSteel_676_240x160.png data/images_dxt
// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))

// Data is bitmap tiles compressed, pixel format: XBGR1555, color map format: Unknown

#pragma once
#include <stdint.h>

#define IMAGES_DXT_WIDTH 240 // width of image in pixels
#define IMAGES_DXT_HEIGHT 160 // height of image in pixels
#define IMAGES_DXT_BYTES_PER_IMAGE 76800 // bytes for one complete image
#define IMAGES_DXT_DATA_SIZE 38867 // size of image data in 4 byte units
#define IMAGES_DXT_NR_OF_IMAGES 12 // # of images in data
extern const uint32_t IMAGES_DXT_DATA_START[IMAGES_DXT_NR_OF_IMAGES]; // indices where data for an image starts (in 4 byte units)
extern const uint32_t IMAGES_DXT_DATA[IMAGES_DXT_DATA_SIZE];
#define IMAGES_DXT_DECOMPRESSION_BUFFER_SIZE 19368 // max. decompression buffer size needed for everything EXCEPT the last step


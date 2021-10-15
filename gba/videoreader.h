#pragma once

#include <cstdint>

namespace VideoReader
{

    struct FileHeader
    {
        uint32_t nrOfFrames = 0;     // Number of frames in file
        uint8_t fps = 0;             // Frames / s. no fractions allowed
        uint16_t width = 0;          // Width in pixels
        uint16_t height = 0;         // Height in pixels
        uint8_t bitsPerPixel = 0;    // Image data bits per pixel (1, 2, 4, 8, 15, 16, 24)
        uint8_t bitsPerColor = 0;    // Color table bits per color (0 - no color table, 15, 16, 24)
        uint8_t colorMapEntries = 0; // Number of color table entries
    } __attribute__((aligned(4), packed));

    /// @brief Get file information from video data
    FileHeader GetFileHeader(const uint8_t *data);

}
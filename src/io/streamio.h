#pragma once

#include "exception.h"
#include "processing/imagestructs.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace IO
{

    class Stream
    {
    public:
        struct FileHeader
        {
            uint32_t nrOfFrames = 0;      // Number of frames in file
            uint16_t width = 0;           // Width in pixels
            uint16_t height = 0;          // Height in pixels
            uint8_t fps = 0;              // Frames / s. no fractions allowed
            uint8_t bitsPerPixel = 0;     // Image data bits per pixel (1, 2, 4, 8, 15, 16, 24)
            uint8_t bitsPerColor = 0;     // Color table bits per color (0 - no color table, 15, 16, 24)
            uint8_t colorMapEntries = 0;  // Number of color table entries
            uint32_t maxMemoryNeeded = 0; // Max. intermediate memory needed to decompress an image. 0 if data can be directly written to destination (single compression stage)
        } __attribute__((aligned(4), packed));

        /// @brief Write frame data to output stream, adding compressed size as 3 byte value at the front
        static auto writeFrame(std::ostream &os, const Image::Data &frame) -> std::ostream &;

        /// @brief Write frame data to output stream, adding compressed size as 3 byte value at the front
        static auto writeFrames(std::ostream &os, const std::vector<Image::Data> &frames) -> std::ostream &;

        /// @brief Write frames to output stream. Will get width / height / color format from first frame in vector
        static auto writeFileHeader(std::ostream &os, const std::vector<Image::Data> &frames, uint8_t fps, uint32_t maxMemoryNeeded) -> std::ostream &;
    };

}

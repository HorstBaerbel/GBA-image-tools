#pragma once

#include "exception.h"
#include "processing/imagestructs.h"

#include <array>
#include <cstdint>
#include <variant>
#include <vector>

namespace IO
{

    class Vid2h
    {
    public:
        static const std::array<char, 4> VID2H_MAGIC; // Expected magic bytes at the start of the file: "v2h" plus a version number, atm "v2h0"

        /// @brief Header for a vid2h binary video stream
        struct FileHeader
        {
            std::array<char, 4> magic;      // Magic bytes at the start of the file: "v2h" plus a version number, atm "v2h0"
            uint32_t nrOfFrames = 0;        // Number of frames in file
            uint16_t width = 0;             // Width in pixels
            uint16_t height = 0;            // Height in pixels
            uint32_t fps = 0;               // Frames / s in 16:15 fixed-point format
            uint8_t bitsPerPixel = 0;       // Image data bits per pixel (1, 2, 4, 8, 15, 16, 24)
            uint8_t bitsPerColor = 0;       // Color table bits per color (0 - no color table, 15, 16, 24)
            uint8_t swappedRedBlue = 0;     // If != 0 red and blue color channels are swapped
            uint8_t colorMapEntries = 0;    // Number of color table entries
            uint32_t videoMemoryNeeded = 0; // Max. intermediate memory needed to decompress an image frame. 0 if data can be directly written to destination (single compression stage)
            uint16_t audioSampleRate = 0;   // Audio sample rate in Hz
            uint8_t audioSampleBits = 0;    // Audio sample bit depth
            uint8_t audioCodec = 0;         // Audio codec used
            uint16_t dummy = 0;
            uint16_t audioMemoryNeeded = 0; // Max. intermediate memory needed to decompress an audio frame. 0 if data can be directly written to destination (single compression stage)
        } __attribute__((aligned(4), packed));

        /// @brief Header for a single frame in a vid2h binary video stream
        struct FrameHeader
        {
            uint32_t pixelDataSize = 0;    // Size of frame pixel data chunk in bytes
            uint16_t colorMapDataSize = 0; // Size of frame colormap data chunk in bytes
            uint16_t audioDataSize = 0;    // Size of frame audio data chunk in bytes
        } __attribute__((aligned(4), packed));

        /// @brief Raw frame data returned when reading a vid2h binary video stream
        struct FrameData
        {
            std::vector<uint8_t> audioData;    // Raw audio data. Might be compressed
            std::vector<uint8_t> pixelData;    // Raw pixel data. Might be compressed
            std::vector<uint8_t> colorMapData; // Raw color map data. Usually uncompressed
        };

        /// @brief Write frame data to output stream, adding compressed size as 4 byte value at the front
        static auto writeFrame(std::ostream &os, const Image::Data &frame) -> std::ostream &;

        /// @brief Write frame data to output stream, adding compressed size as 4 byte value at the front
        static auto writeFrames(std::ostream &os, const std::vector<Image::Data> &frames) -> std::ostream &;

        /// @brief Write frames to output stream. Will get width / height / color format from first frame in vector
        static auto writeFileHeader(std::ostream &os, const std::vector<Image::Data> &frames, double fps, uint32_t videoMemoryNeeded) -> std::ostream &;

        /// @brief Read file header from input stream
        static auto readFileHeader(std::istream &is) -> FileHeader;

        /// @brief Read frame data from input stream
        static auto readFrame(std::istream &is, const FileHeader &fileHeader) -> FrameData;
    };

}

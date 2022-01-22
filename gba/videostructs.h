#pragma once

#include <cstdint>
#include <limits>

namespace Video
{

    /// @brief File header describing the video data
    struct FileHeader
    {
        uint32_t nrOfFrames = 0;     // Number of frames in file
        uint16_t width = 0;          // Width in pixels
        uint16_t height = 0;         // Height in pixels
        uint8_t fps = 0;             // Frames / s. no fractions allowed
        uint8_t bitsPerPixel = 0;    // Image data bits per pixel (1, 2, 4, 8, 15, 16, 24)
        uint8_t bitsInColorMap = 0;  // Color table bits per color (0 - no color table, 15, 16, 24)
        uint8_t colorMapEntries = 0; // Number of color table entries
    } __attribute__((aligned(4), packed));

    /// @brief Video file / data information
    struct Info : public FileHeader
    {
        const uint8_t *fileData = nullptr;  // Pointer to file header data
        const uint8_t *frameData = nullptr; // Pointer to frame data
    } __attribute__((aligned(4), packed));

    /// @brief Chunk of compressed data
    struct DataChunk
    {
        uint32_t uncompressedSize : 24 = 0; // Uncompressed size of data in this chunk
        uint8_t processingType = 0;         // Processing / compression type used on data in this chunk
    } __attribute__((aligned(4), packed));

    /// @brief Frame header describing frame data
    struct Frame
    {
        uint32_t index = std::numeric_limits<uint32_t>::max(); // Frame index in video
        uint32_t chunkOffset = 0;                              // Offset to chunk data in Info::frameData
        uint32_t colorMapOffset = 0;                           // Offset to color map in Info::frameData
        uint32_t compressedSize = 0;                           // Size of frame data in chunk (ONLY frame data, not whole chunk)
    } __attribute__((aligned(4), packed));

}

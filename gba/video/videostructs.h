#pragma once

#include <cstdint>

namespace Video
{

    /// @brief File header describing the video data
    struct FileHeader
    {
        uint32_t nrOfFrames = 0;      // Number of frames in file
        uint16_t width = 0;           // Width in pixels
        uint16_t height = 0;          // Height in pixels
        uint8_t fps = 0;              // Frames / s. no fractions allowed
        uint8_t bitsPerPixel = 0;     // Image data bits per pixel (1, 2, 4, 8, 15, 16, 24)
        uint8_t bitsInColorMap = 0;   // Color table bits per color (0 - no color table, 15, 16, 24)
        uint8_t colorMapEntries = 0;  // Number of color table entries
        uint32_t maxMemoryNeeded = 0; // Max. intermediate memory needed to decompress an image. 0 if data can be directly written to destination (single compression stage)
    } __attribute__((aligned(4), packed));

    /// @brief Video file / data information
    struct Info : public FileHeader
    {
        const uint32_t *fileData = nullptr; // Pointer to file header data
        uint32_t colorMapSize = 0;          // Size of color map data in bytes
    } __attribute__((aligned(4), packed));

    /// @brief Chunk of compressed data
    struct DataChunk
    {
        uint8_t processingType : 8;     // Processing / compression type used on data in this chunk
        uint32_t uncompressedSize : 24; // Uncompressed size of data in this chunk
    } __attribute__((aligned(4), packed));

    /// @brief Frame header describing frame data
    struct Frame
    {
        int32_t index = -1;             // Frame index in video
        const uint32_t *data = nullptr; // Pointer to frame data
        uint32_t colorMapOffset = 0;    // Byte offset to color map in data
        uint32_t compressedSize = 0;    // Size of frame data in chunk (ONLY frame data, not whole chunk)
    } __attribute__((aligned(4), packed));

}

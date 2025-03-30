#pragma once

#include <cstdint>

namespace Video
{

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

    /// @brief Chunk of compressed data
    struct ChunkHeader
    {
        uint8_t processingType : 8;     // Processing / compression type used on data in this chunk
        uint32_t uncompressedSize : 24; // Uncompressed size of data in this chunk
    } __attribute__((aligned(4), packed));

    /// @brief Video file / data information
    struct Info : public FileHeader
    {
        const uint32_t *fileData = nullptr; // Pointer to file header data
        uint32_t colorMapSize = 0;          // Size of color map data in bytes
    } __attribute__((aligned(4), packed));

    /// @brief Frame header describing frame data
    struct Frame
    {
        int32_t index = -1;              // Frame index in video
        const uint32_t *frame = nullptr; // Pointer to frame start
        const uint32_t *data = nullptr;  // Pointer to frame data
        uint32_t pixelDataSize = 0;      // Size of pixel data in bytes
        uint16_t colorMapDataSize = 0;   // Size of colormap data in bytes
        uint16_t audioDataSize = 0;      // Size of audio data in bytes
    } __attribute__((aligned(4), packed));

}

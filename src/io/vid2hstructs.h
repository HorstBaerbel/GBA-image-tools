#pragma once

#include "mediatypes.h"

#include <cstdint>

namespace IO::Vid2h
{

    constexpr uint32_t Magic = 0x76326830; // Expected magic bytes at the start of the file: "v2h" plus a version number, atm "v2h0"

    /// @brief Header for a vid2h binary video stream
    struct FileHeader
    {
        uint32_t magic = 0;               // Magic bytes at the start of the file: "v2h" plus a version number, atm "v2h0"
        FileType contentType : 8;         // Type of content in file
        uint32_t nrOfFrames : 24;         // Number of video and audio frames in file
        uint16_t videoWidth = 0;          // Width in pixels
        uint16_t videoHeight = 0;         // Height in pixels
        uint32_t videoFps = 0;            // Frames / s in 16.16 fixed-point format
        uint8_t videoBitsPerPixel = 0;    // Image data bits per pixel (1, 2, 4, 8, 15, 16, 24)
        uint8_t videoBitsPerColor = 0;    // Color table bits per color (0 - no color table, 15, 16, 24)
        uint8_t videoSwappedRedBlue = 0;  // If != 0 red and blue color channels are swapped
        uint8_t videoColorMapEntries = 0; // Number of color table entries
        uint32_t videoMemoryNeeded = 0;   // Max. intermediate memory needed to decompress an image frame. 0 if data can be directly written to destination (single compression stage)
        uint16_t audioSampleRate = 0;     // Audio sample rate in Hz
        uint8_t audioSampleBits = 0;      // Audio sample bit depth
        uint8_t audioChannels = 0;        // Audio channels used (only 1 or 2 supported)
        int16_t audioOffset = 0;          // Audio offset in comparison to video in signed 0.16 format in s
        uint16_t audioMemoryNeeded = 0;   // Max. intermediate memory needed to decompress an audio frame. 0 if data can be directly written to destination (single compression stage)
    } __attribute__((aligned(4), packed));

    /// @brief Header for a single frame in a vid2h binary video stream
    struct FrameHeader
    {
        FrameType dataType : 8; // Frame data contained
        uint32_t dataSize : 24; // Size of frame pixel / color map / audio data chunk in bytes
    } __attribute__((aligned(4), packed));

    /// @brief Chunk of compressed data
    struct ChunkHeader
    {
        uint8_t processingType : 8;     // Processing / compression type used on data in this chunk
        uint32_t uncompressedSize : 24; // Uncompressed size of data in this chunk
    } __attribute__((aligned(4), packed));

}

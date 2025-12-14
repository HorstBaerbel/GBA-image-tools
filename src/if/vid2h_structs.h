#pragma once

#include "audio_processingtype.h"
#include "image_processingtype.h"
#include "mediatypes.h"

#include <cstdint>

namespace IO::Vid2h
{
    constexpr uint32_t Magic = 0x76326830; // Expected magic bytes at the start of the file: "v2h" plus a version number, atm "v2h0"

    /// @brief Header for a vid2h stream containing video
    struct VideoHeader
    {
        uint16_t nrOfFrames = 0;              // Number of video frames (must not be the same as audio frames)
        uint32_t frameRateHz = 0;             // Video frame rate in Hz in 16.16 fixed-point format
        uint16_t width = 0;                   // Width in pixels
        uint16_t height = 0;                  // Height in pixels
        uint8_t bitsPerPixel = 0;             // Image data bits per pixel (1, 2, 4, 8, 15, 16, 24)
        uint8_t bitsPerColor = 0;             // Color table bits per color (0 - no color table, 15, 16, 24)
        uint8_t colorMapEntries = 0;          // Number of color table entries
        uint8_t swappedRedBlue = 0;           // If != 0 red and blue color channels are swapped
        uint16_t nrOfColorMapFrames = 0;      // Number of colormap frames (must not be the same as video frames)
        uint32_t memoryNeeded = 0;            // Max. intermediate memory needed to decompress an image frame. 0 if data can be directly written to destination (single compression stage)
        Image::ProcessingType processing[4] = // Video processing steps. See image/processingtypes.h
            {Image::ProcessingType::Invalid, Image::ProcessingType::Invalid, Image::ProcessingType::Invalid, Image::ProcessingType::Invalid};
    } __attribute__((packed));

    /// @brief Header for a vid2h stream containing audio
    struct AudioHeader
    {
        uint16_t nrOfFrames = 0;              // Number of audio frames (must not be the same as video frames)
        uint32_t nrOfSamples = 0;             // Number of audio samples per channel
        uint16_t sampleRateHz = 0;            // Audio sample rate in Hz
        uint8_t channels = 0;                 // Audio channels used (only 1 or 2 supported)
        uint8_t sampleBits = 0;               // Audio sample bit depth (8, 16), always a signed data type
        int16_t offsetSamples = 0;            // Audio offset in comparison to video in # of samples
        uint16_t memoryNeeded = 0;            // Max. intermediate memory needed to decompress an audio frame. 0 if data can be directly written to destination (single compression stage)
        uint16_t dummy = 0;                   // Padding so size is multiple of 4
        Audio::ProcessingType processing[4] = // Audio processing steps. See audio/processingtypes.h
            {Audio::ProcessingType::Invalid, Audio::ProcessingType::Invalid, Audio::ProcessingType::Invalid, Audio::ProcessingType::Invalid};
    } __attribute__((packed));

    /// @brief Header for a vid2h stream containing subtitles
    struct SubtitlesHeader
    {
        uint16_t nrOfFrames = 0; // Number of subtitle frames (must not be the same as video frames)
        uint16_t dummy = 0;      // Padding so size is multiple of 4
    } __attribute__((packed));

    /// @brief Header for a vid2h binary video stream
    struct FileHeader
    {
        uint32_t magic = 0;                       // Magic bytes at the start of the file: "v2h" plus a version number, atm "v2h0"
        FileType contentType = FileType::Unknown; // Type of content
        uint8_t dummy = 0;                        // Padding so size is multiple of 4
        uint16_t metaDataSize = 0;                // Size of unstructured meta data at the end of the file!
    } __attribute__((aligned(4), packed));
    // * after this follow the audio (if file contains audio) and video header (if file contains video)
    // * then follow the actual data frames
    // * at the end of the file follows meta data. If it is empty (metaDataSize == 0), it is 0 bytes long

    /// @brief Header for a single frame in a vid2h binary video stream
    struct FrameHeader
    {
        FrameType dataType : 8; // Frame data contained
        uint32_t dataSize : 24; // Size of frame pixel / color map / audio data chunk in bytes
    } __attribute__((aligned(4), packed));
}

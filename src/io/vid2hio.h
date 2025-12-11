#pragma once

#include "audio/audiostructs.h"
#include "exception.h"
#include "if/audio_processingtype.h"
#include "if/image_processingtype.h"
#include "if/vid2h_structs.h"
#include "image/imagestructs.h"

#include <array>
#include <cstdint>
#include <variant>
#include <vector>

namespace IO::Vid2h
{

    /// @brief Information about the file header
    struct FileDataInfo
    {
        FileType contentType = IO::FileType::Unknown; // File content type
        int32_t audioHeaderOffset = -1;               // Offset to audio header data in bytes
        int32_t videoHeaderOffset = -1;               // Offset to video header data in bytes
        int32_t frameDataOffset = -1;                 // Offset to frame data in bytes
        int32_t metaDataOffset = -1;                  // Offset to meta data in bytes
    };

    // --------------------------------------------------------------------------------------------------------

    /// @brief Write initial dummy frame header to output stream. Use to inialize file. Then rewind os and use write*Header() to write a proper header when you have all the information
    /// @param os Output stream
    /// @param contentType content type to write to file
    /// @return Header information
    /// @note This moves the os stream position past the file header
    auto writeFileHeader(std::ostream &os, const FileType contentType) -> FileDataInfo;

    /// @brief Create video header
    auto createVideoHeader(const Image::FrameInfo &imageInfo, uint32_t videoNrOfFrames, double videoFrameRateHz, uint32_t videoMemoryNeeded, uint32_t videoNrOfColorMapFrames, const std::vector<Image::ProcessingType> &decodingSteps) -> VideoHeader;

    /// @brief Create audio header
    auto createAudioHeader(const Audio::FrameInfo &audioInfo, uint32_t audioNrOfFrames, uint32_t audioNrOfSamples, int32_t audioOffsetSamples, uint32_t audioMemoryNeeded, const std::vector<Audio::ProcessingType> &decodingSteps) -> AudioHeader;

    /// @brief Write video file header to output stream
    /// @note This moves the os stream position past the video header
    auto writeVideoHeader(std::ostream &os, const FileDataInfo &fileHeaderInfo, const VideoHeader &videoHeader) -> void;

    /// @brief Write audio file header to output stream
    /// @note This moves the os stream position past the audio header
    auto writeAudioHeader(std::ostream &os, const FileDataInfo &fileHeaderInfo, const AudioHeader &audioHeader) -> void;

    /// @brief Write video frame data to output stream, adding compressed size as 4 byte value at the front
    /// @note This advances the os stream position past the new frame data
    auto writeFrame(std::ostream &os, const Image::Frame &frame) -> void;

    /// @brief Write audio frame data to output stream, adding compressed size as 4 byte value at the front
    /// @note This advances the os stream position past the new frame data
    auto writeFrame(std::ostream &os, const Audio::Frame &frame) -> void;

    /// @brief Write meta data at the end of the output stream and adjust the file header value metaDataSize accordingly
    /// @note Use this AFTER writing all data frames using writeFrame()! This advances the os stream position past the new meta data
    auto writeMetaData(std::ostream &os, const FileDataInfo &fileHeaderInfo, const std::vector<uint8_t> &metaData) -> void;

    // --------------------------------------------------------------------------------------------------------

    /// @brief Read file header from input stream
    auto readFileHeader(std::istream &is) -> FileDataInfo;

    /// @brief Read video header from stream
    auto readVideoHeader(std::istream &is, const FileDataInfo &fileHeaderInfo) -> VideoHeader;

    /// @brief Read audio header from stream
    auto readAudioHeader(std::istream &is, const FileDataInfo &fileHeaderInfo) -> AudioHeader;

    /// @brief Read frame data from input stream
    auto readFrame(std::istream &is) -> std::pair<IO::FrameType, std::vector<uint8_t>>;

    /// @brief Read meta data from end of stream
    auto readMetaData(std::istream &is, const FileDataInfo &fileHeaderInfo) -> std::vector<uint8_t>;
}

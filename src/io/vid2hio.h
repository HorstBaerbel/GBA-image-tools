#pragma once

#include "audio/audiostructs.h"
#include "audio/processingtype.h"
#include "exception.h"
#include "image/imagestructs.h"
#include "image/processingtype.h"
#include "vid2hstructs.h"

#include <array>
#include <cstdint>
#include <variant>
#include <vector>

namespace IO::Vid2h
{
    /// @brief Write dummy frame header to output stream. Use to inialize file. Then rewind and use writeFileHeader() to write a proper header when you have all the information
    auto writeDummyFileHeader(std::ostream &os) -> std::ostream &;

    /// @brief Create video-only header
    auto createVideoHeader(const Image::FrameInfo &imageInfo, uint32_t videoNrOfFrames, double videoFrameRateHz, uint32_t videoMemoryNeeded, uint32_t videoNrOfColorMapFrames, const std::vector<Image::ProcessingType> &decodingSteps) -> VideoHeader;

    /// @brief Create audio-only header
    auto createAudioHeader(const Audio::FrameInfo &audioInfo, uint32_t audioNrOfFrames, uint32_t audioNrOfSamples, int32_t audioOffsetSamples, uint32_t audioMemoryNeeded, const std::vector<Audio::ProcessingType> &decodingSteps) -> AudioHeader;

    /// @brief Write video-only file header to output stream
    auto writeVideoFileHeader(std::ostream &os, const VideoHeader &videoHeader) -> std::ostream &;

    /// @brief Write audio-only file header to output stream
    auto writeAudioFileHeader(std::ostream &os, const AudioHeader &audioHeader) -> std::ostream &;

    /// @brief Write audio+video file header to output stream
    auto writeMediaFileHeader(std::ostream &os, const VideoHeader &videoHeader, const AudioHeader &audioHeader) -> std::ostream &;

    /// @brief Write video frame data to output stream, adding compressed size as 4 byte value at the front
    auto writeFrame(std::ostream &os, const Image::Frame &frame) -> std::ostream &;

    /// @brief Write audio frame data to output stream, adding compressed size as 4 byte value at the front
    auto writeFrame(std::ostream &os, const Audio::Frame &frame) -> std::ostream &;

    /// @brief Read file header from input stream
    auto readFileHeader(std::istream &is) -> FileHeader;

    /// @brief Read frame data from input stream
    auto readFrame(std::istream &is, const FileHeader &fileHeader) -> std::pair<IO::FrameType, std::vector<uint8_t>>;
}

#pragma once

#include "vid2hstructs.h"
#include "exception.h"
#include "processing/imagestructs.h"

#include <array>
#include <cstdint>
#include <variant>
#include <vector>

namespace IO::Vid2h
{
    /// @brief Write dummy frame header to output stream. Use to inialize file. Then rewind and use writeFileHeader() to write a proper header when you have all the information
    auto writeDummyFileHeader(std::ostream &os) -> std::ostream &;

    /// @brief Write frame header to output stream. Will get width / height / color format from first frame in vector
    auto writeFileHeader(std::ostream &os, const Image::ImageInfo &frameInfo, uint32_t nrOfFrames, double fps, uint32_t videoMemoryNeeded) -> std::ostream &;

    /// @brief Write video frame data to output stream, adding compressed size as 4 byte value at the front
    auto writeVideoFrame(std::ostream &os, const Image::Data &frame) -> std::ostream &;

    /// @brief Write audio frame data to output stream, adding compressed size as 4 byte value at the front
    auto writeAudioFrame(std::ostream &os, const std::vector<uint8_t> &frame, uint32_t index) -> std::ostream &;

    /// @brief Read file header from input stream
    auto readFileHeader(std::istream &is) -> FileHeader;

    /// @brief Read frame data from input stream
    auto readFrame(std::istream &is, const FileHeader &fileHeader) -> std::pair<IO::FrameType, std::vector<uint8_t>>;

    /// @brief Split data into chunk header and chunk data
    auto splitChunk(std::vector<uint8_t> &data) -> std::pair<ChunkHeader, std::vector<uint8_t>>;
}

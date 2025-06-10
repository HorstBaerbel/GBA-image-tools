#pragma once

#include "io/vid2hstructs.h"

#include <cstdint>

namespace IO::Vid2h
{

    /// @brief Video file / data information
    struct Info : public FileHeader
    {
        const uint32_t *fileData = nullptr; // Pointer to file header data
        uint32_t imageSize = 0;             // Size of image data in bytes
        uint32_t colorMapSize = 0;          // Size of color map data in bytes
    } __attribute__((aligned(4), packed));

    /// @brief Frame header describing frame data
    struct Frame : public FrameHeader
    {
        int32_t index = -1;               // Frame index in video
        const uint32_t *header = nullptr; // Pointer to frame start / header
        const uint32_t *data = nullptr;   // Pointer to frame data
    } __attribute__((aligned(4), packed));

    /// @brief Get static file information from video data
    /// @param data Pointer to start of file data
    Info GetInfo(const uint32_t *data);

    /// @brief Get frame following previous frame
    /// @param info File data information. Read with GetInfo()
    /// @param previous Previous frame. When starting to read frames, insert an empty Frame here
    /// @note Will return the first frame passing the last frame in previousFrame
    Frame GetNextFrame(const Info &info, const Frame &previous);
}
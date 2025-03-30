#pragma once

#include "videostructs.h"

#include <cstdint>

namespace Video
{

    /// @brief Get static file information from video data
    /// @param data Pointer to start of file data
    Info GetInfo(const uint32_t *data);

    /// @brief Get frame following previous frame
    /// @param info File data information. Read with GetInfo()
    /// @param previous Previous frame. When starting to read frames, insert an empty Frame here
    /// @note Will return the first frame passing the last frame in previousFrame
    FrameData GetNextFrame(const Info &info, const FrameData &previous);
}
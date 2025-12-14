#pragma once

#include <cstdint>
#include <string>

namespace Subtitles
{

    /// @brief Subtitles data returned from media reader
    struct RawData
    {
        uint16_t durationFrames = 0; // Duration of the subtitle in frames
        std::string text;
    };

    constexpr uint32_t MaxSubTitleLength = 64;

    /// @brief Stores data for a subtitles frame
    struct Frame
    {
        uint32_t index = 0;          // Input subtitle index counter
        uint32_t durationFrames = 0; // Duration of the subtitle in video frames
        std::string text;
    };

}

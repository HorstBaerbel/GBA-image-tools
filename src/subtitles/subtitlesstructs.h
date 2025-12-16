#pragma once

#include <cstdint>
#include <string>

namespace Subtitles
{
    /// @brief Subtitles data returned from media reader
    struct RawData
    {
        double startTimeS = 0; // Start time in seconds
        double endTimeS = 0;   // End time in seconds
        std::string text;
    };

    constexpr uint32_t MaxSubTitleLength = 64;

    /// @brief One subtitle from a SRT file
    struct Frame
    {
        uint32_t index = 0;    // Index in file
        double startTimeS = 0; // Start time in seconds
        double endTimeS = 0;   // End time in seconds
        std::string text;      // Subtitle text
    };
}

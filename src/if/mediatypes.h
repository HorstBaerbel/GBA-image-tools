#pragma once

#include <cstdint>

namespace IO
{

    /// @brief Bitfield defining what type of media the file contains
    enum FileType : uint8_t
    {
        Unknown = 0,      // Bad content type
        Audio = 0x01,     // File contains audio data
        Video = 0x02,     // File contains video data
        AudioVideo = 0x03 // File contains both video and audio data
    };

    /// @brief Frame content type
    enum class FrameType : uint8_t
    {
        Unknown = 0,  // Bad frame type
        Pixels = 1,   // Pixel data
        Colormap = 2, // Color map data
        Audio = 3,    // Audio data
        Meta = 4      // Meta data
    };
}
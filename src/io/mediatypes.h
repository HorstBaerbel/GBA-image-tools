#pragma once

#include <cstdint>

namespace IO
{

    /// @brief Bitfield defining what type of media the file contains
    enum FileType : uint8_t
    {
        Unknown = 0,  // Bad content type
        Video = 0x01, // File contains video data
        Audio = 0x02  // File contains audio data
    };

    /// @brief Frame content type
    enum class FrameType : uint8_t
    {
        Unknown = 0,  // Bad frame type
        Pixels = 1,   // Pixel data
        Colormap = 2, // Color map data
        Audio = 3,    // Audio data
    };

}
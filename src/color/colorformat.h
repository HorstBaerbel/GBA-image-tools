#pragma once

#include <cstdint>
#include <string>

namespace Color
{

    /// @brief Color format information
    enum class Format
    {
        Unknown = 0,
        Paletted1 = 1, // 1bit paletted b/w format
        Paletted2 = 2, // 2bit paletted format
        Paletted4 = 4, // 4bit paletted format
        Paletted8 = 8, // 8bit paletted format
        RGB555 = 15,   // R5G5B5 16-bit format for GBA
        RGB565 = 16,   // R5G6B5 16-bit format for NDS, DXT
        RGB888 = 24,   // R8G8B8 24-bit straight truecolor format
        XRGB888 = 32,  // X8R8G8B8 32-bit straight truecolor format
        RGBf = 96,     // RGB float truecolor format
        YCgCof = 97    // YCgCo float truecolor format
    };

    /// @brief Return bits per pixel for input color format
    uint32_t bitsPerPixelForFormat(Format format);

    /// @brief Return color format as string
    std::string to_string(Format format);

}

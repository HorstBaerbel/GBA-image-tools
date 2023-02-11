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
        RGB555 = 15,   // RGB555 format for GBA
        RGB565 = 16,   // RGB565 format for NDS, DXT
        RGB888 = 24,   // RGB888 straight truecolor format
        RGBd = 192,    // RGB double truecolor format
        YCgCod = 193   // YCgCo double truecolor format
    };

    /// @brief Return bits per pixel for input color format
    uint32_t bitsPerPixelForFormat(Format format);

    /// @brief Return color format as string
    std::string to_string(Format format);

}

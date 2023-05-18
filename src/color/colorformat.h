#pragma once

#include <cstdint>
#include <string>

namespace Color
{

    /// @brief Color format information
    enum class Format
    {
        Unknown = 0,   // Bad, raw or compressed data
        Paletted1 = 1, // 1bit paletted b/w format
        Paletted2 = 2, // 2bit paletted format
        Paletted4 = 4, // 4bit paletted format
        Paletted8 = 8, // 8bit paletted format
        XRGB1555 = 15, // X1R5G5B5 16-bit format for GBA
        RGB565 = 16,   // R5G6B5 16-bit format for NDS, DXT
        XRGB8888 = 32, // X8R8G8B8 32-bit straight truecolor format
        LChf = 96,     // LCh float truecolor format
        RGBf = 97,     // RGB float truecolor format
        YCgCoRf = 98   // YCgCoR float truecolor format
    };

    /// @brief Return true if color format is indexed / paletted
    auto isIndexed(Format format) -> bool;

    /// @brief Return true if color format is truecolor
    auto isTruecolor(Format format) -> bool;

    /// @brief Return bits per pixel for input color format
    auto bitsPerPixelForFormat(Format format) -> uint32_t;

    /// @brief Return bits per pixel for input color format
    auto bytesPerColorMapEntry(Format format) -> uint32_t;

    /// @brief Return color format as string
    auto toString(Format format) -> std::string;

    /// @brief Return color format for color value type
    /// @note Paletted and raw types can not be distinguished as they are all uint8_t and will return "Unknown"
    template <typename PIXEL_TYPE>
    auto toFormat() -> Format;

}

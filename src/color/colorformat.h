#pragma once

#include <cstdint>
#include <string>

namespace Color
{

    /// @brief Color format identifier
    enum class Format : uint8_t
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
        YCgCoRf = 98,  // YCgCoR float truecolor format
        Grayf = 111    // Single-channel float grayscale format
    };

    /// @brief Color format information
    struct FormatInfo
    {
        Format format = Format::Unknown;
        std::string name;           // Color format as string
        uint32_t bitsPerPixel = 0;  // Bits per pixel for input color format
        uint32_t bytesPerPixel = 0; // Bytes per pixel for input color format
        uint32_t channels = 0;      // Color channels in color data
        bool isIndexed = false;     // If color format is indexed / paletted
        bool isTruecolor = false;   // If color format is truecolor
    };

    /// @brief Return color format information
    auto formatInfo(Format format) -> const FormatInfo &;

    /// @brief Return color format for color value type
    /// @note Paletted and raw types can not be distinguished as they are all uint8_t and will return "Unknown"
    template <typename PIXEL_TYPE>
    auto toFormat() -> Format;

}

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
        XRGB1555 = 15, // X1R5G5B5 16-bit format
        RGB565 = 16,   // R5G6B5 16-bit format
        XBGR1555 = 17, // X1B5G5R5 16-bit format with swapped red and blue for GBA (has no Color:: class)
        BGR565 = 18,   // B5G6R5 16-bit format with swappe red and blue for NDS, DXT (has no Color:: class)
        RGB888 = 24,   // R8G8B8 24-bit straight truecolor format
        BGR888 = 25,   // B8G8R8 24-bit straight truecolor format with swapped red and blue (has no Color:: class)
        XRGB8888 = 32, // X8R8G8B8 32-bit straight truecolor format
        XBGR8888 = 33, // X8R8G8B8 32-bit straight truecolor format with swapped red and blue (has no Color:: class)
        RGBf = 96,     // RGB float truecolor format
        LChf = 97,     // LCh float truecolor format
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
        bool isIndexed = false;     // True if color format is indexed / paletted
        bool isTruecolor = false;   // True if color format is truecolor
        bool hasAlpha = false;      // True if color format has alpah channel
        bool hasSwappedRedBlue = false; // True if the color has swapped red and blue component
    };

    /// @brief Return color format information
    auto formatInfo(Format format) -> const FormatInfo &;

    /// @brief Return color format for color value type
    /// @note Paletted and raw types can not be distinguished as they are all uint8_t and will return "Unknown"
    template <typename PIXEL_TYPE>
    auto toFormat() -> Format;

}

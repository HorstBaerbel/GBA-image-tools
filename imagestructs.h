#pragma once

#include "exception.h"
#include "datahelpers.h"

#include <variant>
#include <cstdint>
#include <vector>
#include <Magick++.h>

namespace Image
{

    // Color format info
    enum class ColorFormat
    {
        Unknown,
        Paletted1 = 1, // 1bit paletted b/w format
        Paletted2 = 2, // 2bit paletted format
        Paletted4 = 4, // 4bit paletted format
        Paletted8 = 8, // 8bit paletted format
        RGB555 = 15,   // RGB555 GBA format
        RGB565 = 16,   // RGB565 format for DXT
        RGB888 = 24    // RGB888 straight truecolor format
    };

    /// @brief Return bits per pixel for input color format
    uint32_t bitsPerPixelForFormat(ColorFormat format);

    /// @brief Stores data for an image
    struct Data
    {
        std::string fileName;                // input file name
        Magick::ImageType type;              // input image type
        Magick::Geometry size;               // image size
        ColorFormat format;                  // image data format
        std::vector<uint8_t> data;           // raw image data
        std::vector<Magick::Color> colorMap; // image color map if paletted
        ColorFormat colorMapFormat;          // raw color map data format
        std::vector<uint8_t> colorMapData;   // raw color map data
    };

}

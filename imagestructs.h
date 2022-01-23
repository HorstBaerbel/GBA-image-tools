#pragma once

#include "datahelpers.h"
#include "exception.h"

#include <Magick++.h>
#include <cstdint>
#include <variant>
#include <vector>

namespace Image
{

    /// @brief Color format info
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

    /// @brief Return color format as string
    std::string to_string(ColorFormat format);

    /// @brief Type of data currently stored in image data
    enum class DataType
    {
        Unknown,
        Bitmap, // image / bitmap data
        Tilemap // tilemap data
    };

    /// @brief Stores data for an image
    struct Data
    {
        std::string fileName;                // input file name
        Magick::ImageType type;              // input image type
        Magick::Geometry size;               // image size
        DataType dataType;                   // image data type
        ColorFormat colorFormat;             // image color format
        std::vector<uint16_t> mapData;       // raw screen / map data (only if dataType == Tilemap)
        std::vector<uint8_t> data;           // raw image / bitmap / tile data
        std::vector<Magick::Color> colorMap; // image color map if paletted
        ColorFormat colorMapFormat;          // raw color map data format
        std::vector<uint8_t> colorMapData;   // raw color map data
    };

}

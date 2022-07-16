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
        RGB555 = 15,   // RGB555 format
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
        uint32_t index = 0;                                        // image index counter
        std::string fileName;                                      // input file name
        Magick::ImageType type = Magick::ImageType::UndefinedType; // input image type
        Magick::Geometry size = {0, 0};                            // image size
        DataType dataType = DataType::Unknown;                     // image data type
        ColorFormat colorFormat = ColorFormat::Unknown;            // image color format
        std::vector<uint16_t> mapData;                             // raw screen / map data (only if dataType == Tilemap)
        std::vector<uint8_t> data;                                 // raw image / bitmap / tile data
        std::vector<Magick::Color> colorMap;                       // image color map if paletted
        ColorFormat colorMapFormat = ColorFormat::Unknown;         // raw color map data format
        std::vector<uint8_t> colorMapData;                         // raw color map data
        uint32_t maxMemoryNeeded = 0;                              // max. intermediate memory needed to process the image. 0 if it can be directly written to destination (single processing stage)
    };

    /// @brief Return true if the data has a color map, false if not.
    auto hasColorMap(const Data &frame) -> bool;

    /// @brief Return number of full bytes needed per color map entry.
    auto bytesPerColorMapEntry(const Data &frame) -> uint32_t;

}

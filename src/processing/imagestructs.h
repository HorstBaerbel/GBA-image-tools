#pragma once

#include "color/colorformat.h"
#include "datahelpers.h"
#include "datasize.h"
#include "exception.h"

#include <Magick++.h>
#include <cstdint>
#include <variant>
#include <vector>

namespace Image
{

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
        DataSize size = {0, 0};                                    // image size
        DataType dataType = DataType::Unknown;                     // image data type
        Color::Format colorFormat = Color::Format::Unknown;        // image color format
        std::vector<uint16_t> mapData;                             // raw screen / map data (only if dataType == Tilemap)
        std::vector<uint8_t> data;                                 // raw image / bitmap / tile data
        std::vector<Magick::Color> colorMap;                       // image color map if paletted
        Color::Format colorMapFormat = Color::Format::Unknown;     // raw color map data format
        std::vector<uint8_t> colorMapData;                         // raw color map data
        uint32_t maxMemoryNeeded = 0;                              // max. intermediate memory needed to process the image. 0 if it can be directly written to destination (single processing stage)
    };

    /// @brief Return true if the data has paletted data (1/2/4/8 bits), false if not.
    auto isPaletted(const Data &frame) -> bool;

    /// @brief Return true if the data has a color map, false if not.
    auto hasColorMap(const Data &frame) -> bool;

    /// @brief Return number of full bytes needed per color map entry.
    auto bytesPerColorMapEntry(const Data &frame) -> uint32_t;
}

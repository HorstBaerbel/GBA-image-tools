#pragma once

#include "datahelpers.h"
#include "datasize.h"
#include "datatype.h"
#include "exception.h"
#include "imagedata.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace Image
{

    using MapData = std::vector<uint16_t>; // Screen / map data that specifies which tile index is displayed at a screen position

    /// @brief Information about tile map
    struct MapInfo
    {
        DataSize size = {0, 0}; // Size of map data (if filled)
        MapData data;           // Raw screen / tile map data
    };

    /// @brief Information about current / final image data before compression / conversion to raw
    struct ImageInfo
    {
        DataSize size = {0, 0};                                // Size of image or sprites (if sprites)
        Color::Format pixelFormat = Color::Format::Unknown;    // The current / final image pixel format before e.g. compression / conversion to raw
        Color::Format colorMapFormat = Color::Format::Unknown; // The current / final image color map format before e.g. compression / conversion to raw
        uint32_t nrOfColorMapEntries = 0;                      // The current / final number of color map entries before e.g. compression / conversion to raw
        ImageData data;                                        // Image / bitmap / sprite / tile data. indexed, true color or raw / compressed data
        uint32_t maxMemoryNeeded = 0;                          // Max. intermediate memory needed to process the image. 0 if it can be directly written to destination (single processing stage)
    };

    /// @brief Stores data for an image
    struct Data
    {
        uint32_t index = 0;   // Input image index counter
        std::string fileName; // Input file name
        DataType type;        // The type of data stored
        ImageInfo image;      // Image data
        MapInfo map;          // Screen / tile map data (if any)
    };

    /// @brief Return number of bits needed per color pixel.
    auto bitsPerPixel(const Data &image) -> uint32_t;

    /// @brief Return number of full bytes needed per color pixel.
    auto bytesPerPixel(const Data &image) -> uint32_t;

    /// @brief Return number of bits needed per color map entry.
    auto bitsPerColorMapEntry(const Data &image) -> uint32_t;

    /// @brief Return number of full bytes needed per color map entry.
    auto bytesPerColorMapEntry(const Data &image) -> uint32_t;
}

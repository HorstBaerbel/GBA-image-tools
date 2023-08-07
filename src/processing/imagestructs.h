#pragma once

#include "datahelpers.h"
#include "datasize.h"
#include "exception.h"
#include "imagedata.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace Image
{

    /// @brief Type of data currently stored in data
    enum class DataType
    {
        Unknown,
        Bitmap, // Image / bitmap data
        Tilemap // Tilemap data
    };

    using MapData = std::vector<uint16_t>; // Screen / map data that specifies which tile index is displayed at a screen position

    /// @brief Stores data for an image
    struct Data
    {
        uint32_t index = 0;                    // image index counter
        std::string fileName;                  // input file name
        DataSize size = {0, 0};                // image size
        DataType dataType = DataType::Unknown; // image data type
        MapData mapData;                       // raw screen / map data (only if dataType == Tilemap)
        ImageData imageData;                   // image / bitmap / tile data. indexed, true color or raw / compressed data
        uint32_t maxMemoryNeeded = 0;          // max. intermediate memory needed to process the image. 0 if it can be directly written to destination (single processing stage)
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

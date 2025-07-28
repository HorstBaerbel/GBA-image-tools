#pragma once

#include "color/colorformat.h"
#include "exception.h"
#include "imagedata.h"
#include "imagestructs.h"
#include "processing/datahelpers.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace Image
{

    /// @brief Combine raw image pixel data of all images and return the data and the start indices into that data.
    /// Indices are returned in OUT_TYPE units
    template <typename OUT_TYPE>
    std::pair<std::vector<OUT_TYPE>, std::vector<uint32_t>> combineRawPixelData(const std::vector<Frame> &images, bool interleavePixels = false)
    {
        std::vector<std::vector<uint8_t>> temp8;
        std::transform(images.cbegin(), images.cend(), std::back_inserter(temp8), [](const auto &img)
                       { return img.data.pixels().convertDataToRaw(); });
        if (interleavePixels)
        {
            const auto allDataSameSize = std::find_if_not(temp8.cbegin(), temp8.cend(), [refSize = temp8.front().size()](const auto &d)
                                                          { return d.size() == refSize; }) == temp8.cend();
            REQUIRE(allDataSameSize, std::runtime_error, "The image pixel data size of all images must be the same for interleaving");
            return {DataHelpers::convertTo<OUT_TYPE>(DataHelpers::interleave(temp8, Color::formatInfo(images.front().data.pixels().format()).bitsPerPixel)), std::vector<uint32_t>()};
        }
        else
        {
            auto startIndices = DataHelpers::getStartIndices(temp8);
            for (auto index : startIndices)
            {
                REQUIRE(index % sizeof(OUT_TYPE) == 0, std::runtime_error, "The image pixel data size of all images must be evenly dividable by " << sizeof(OUT_TYPE) / sizeof(uint8_t));
            }
            return {DataHelpers::combineTo<OUT_TYPE>(temp8), DataHelpers::divideBy<uint32_t>(startIndices, sizeof(OUT_TYPE) / sizeof(uint8_t))};
        }
    }

    /// @brief Combine raw map data of all images in uint16_t units and return the data and the start indices into that data.
    /// Indices are return in OUT_TYPE units
    template <typename OUT_TYPE>
    std::pair<std::vector<OUT_TYPE>, std::vector<uint32_t>> combineRawMapData(const std::vector<Frame> &images)
    {
        std::vector<std::vector<uint8_t>> temp8;
        std::transform(images.cbegin(), images.cend(), std::back_inserter(temp8), [](const auto &img)
                       { return DataHelpers::convertTo<uint8_t>(img.map.data); });
        auto startIndices = DataHelpers::getStartIndices(temp8);
        for (auto index : startIndices)
        {
            REQUIRE(index % sizeof(OUT_TYPE) == 0, std::runtime_error, "The image map data size of all images must be evenly dividable by " << sizeof(OUT_TYPE) / sizeof(uint8_t));
        }
        return {DataHelpers::combineTo<OUT_TYPE>(temp8), DataHelpers::divideBy<uint32_t>(DataHelpers::getStartIndices(temp8), sizeof(OUT_TYPE) / sizeof(uint8_t))};
    }

    /// @brief Combine the raw image color map data of all images and return the data and the start indices into that data.
    /// Indices are returned in OUT_TYPE units
    template <typename OUT_TYPE>
    std::pair<std::vector<OUT_TYPE>, std::vector<uint32_t>> combineRawColorMapData(const std::vector<Frame> &images)
    {
        std::vector<std::vector<uint8_t>> temp8;
        std::transform(images.cbegin(), images.cend(), std::back_inserter(temp8), [](const auto &img)
                       { return img.data.colorMap().convertDataToRaw(); });
        auto startIndices = DataHelpers::getStartIndices(temp8);
        for (auto index : startIndices)
        {
            REQUIRE(index % sizeof(OUT_TYPE) == 0, std::runtime_error, "The image pixel data size of all images must be evenly dividable by " << sizeof(OUT_TYPE) / sizeof(uint8_t));
        }
        return {DataHelpers::combineTo<OUT_TYPE>(temp8), DataHelpers::divideBy<uint32_t>(startIndices, sizeof(OUT_TYPE) / sizeof(uint8_t))};
    }

}

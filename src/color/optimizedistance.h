#pragma once

#include "color/conversions.h"
#include "color/cielabf.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>

namespace ColorHelpers
{

    /// @brief Find optimal insertion position for color according to color distance map
    auto insertIndexOptimal(const std::vector<uint8_t> &indices, const std::map<uint8_t, std::vector<float>> &distancesSqrMap, uint8_t indexToInsert) -> std::vector<uint8_t>;

    /// @brief Reorder colors to optimize / minimize preceived color distance using CIELab color space distance
    /// Sorts by a, then by b, then by lightness
    /// @return Returns the new order of indices: old_index -> new_index
    template <typename T>
    auto optimizeColorDistance(const std::vector<T> &colors) -> std::vector<uint8_t>
    {
        // convert all colors to CIELab color space
        std::vector<Color::CIELabf> labColors;
        std::transform(colors.cbegin(), colors.cend(), std::back_inserter(labColors), [](const auto &c)
                       { return convertTo<Color::CIELabf>(c); });
        // build map with color distance for all possible combinations from palette
        std::map<uint8_t, std::vector<float>> distancesSqrMap;
        for (uint32_t i = 0; i < labColors.size(); i++)
        {
            const auto &a = labColors.at(i);
            std::vector<float> distances;
            for (const auto &b : labColors)
            {
                distances.push_back(Color::CIELabf::mse(a, b));
            }
            distancesSqrMap[i] = distances;
        }
        // sort color indices
        std::vector<uint8_t> sortedIndices;
        std::generate_n(std::back_inserter(sortedIndices), labColors.size(), [i = 0]() mutable
                        { return i++; });
        constexpr float epsilon = 0.1F;
        std::sort(sortedIndices.begin(), sortedIndices.end(), [labColors](auto ia, auto ib)
                  {
                  const auto &ca = labColors.at(ia);
                  const auto &cb = labColors.at(ib);
                  auto dista = std::abs(cb.a() - ca.a());
                  auto distb = std::abs(cb.b() - ca.b());
                  auto distL = std::abs(cb.L() - ca.L());
                  return (dista > epsilon && distb > epsilon && distL > epsilon) ||
                         (dista < epsilon && distb > epsilon && distL > epsilon) ||
                         (dista < epsilon && distb < epsilon && distL > epsilon); });
        // insert colors / indices successively at optimal positions
        std::vector<uint8_t> currentIndices(1, sortedIndices.front());
        for (uint32_t i = 1; i < sortedIndices.size(); i++)
        {
            currentIndices = insertIndexOptimal(currentIndices, distancesSqrMap, sortedIndices.at(i));
        }
        return currentIndices;
    }
}

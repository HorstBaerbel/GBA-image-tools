#pragma once

#include "color/lchf.h"
#include "color/conversions.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <vector>

namespace Color
{

    /// @brief Calculate square of distance between colors (scalar product)
    /// @return Returns average block color distance in [0,1]
    template <typename T, std::size_t N>
    static auto distance(const std::array<T, N> &colors0, const std::array<T, N> &colors1) -> float
    {
        float dist = 0.0F;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
        {
            dist += distance(*c0It, *c1It);
        }
        return dist / N;
    }

    /// @brief Calculate square of distance between colors (scalar product) and if there are are outliers above a threshold
    /// @return Returns (true if all colors below threshold, average color distance in [0,1])
    template <typename T, std::size_t N>
    static auto distanceBelowThreshold(const std::array<T, N> &colors0, const std::array<T, N> &colors1, float threshold) -> std::pair<bool, float>
    {
        bool belowThreshold = true;
        float dist = 0.0F;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
        {
            auto colorDist = distance(*c0It, *c1It);
            belowThreshold = belowThreshold ? colorDist < threshold : belowThreshold;
            dist += colorDist;
        }
        return {belowThreshold, dist / N};
    }

    /// @brief Calculate RMS distance for color index constellation
    auto calculateDistanceRMS(const std::vector<uint8_t> &indices, const std::map<uint8_t, std::vector<float>> &distancesSqrMap) -> float;

    /// @brief Find optimal insertion position for color according to color distance map
    auto insertIndexOptimal(const std::vector<uint8_t> &indices, const std::map<uint8_t, std::vector<float>> &distancesSqrMap, uint8_t indexToInsert) -> std::vector<uint8_t>;

    /// @brief Reorder colors to minimize preceived color distance using Lch color space distance
    template <typename T>
    auto minimizeColorDistance(const std::vector<T> &colors) -> std::vector<uint8_t>
    {
        // convert all colors to Lch color space
        std::vector<T> lchColors;
        std::transform(colors.cbegin(), colors.cend(), std::back_inserter(lchColors), [](const auto &c)
                       { return convertTo<Lchf>(c); });
        // build map with color distance for all possible combinations from palette
        std::map<uint8_t, std::vector<float>> distancesSqrMap;
        for (uint32_t i = 0; i < lchColors.size(); i++)
        {
            const auto &a = lchColors.at(i);
            std::vector<float> distances;
            for (const auto &b : lchColors)
            {
                distances.push_back(distance(a, b));
            }
            distancesSqrMap[i] = distances;
        }
        // sort color indices by hue and lightness first
        std::vector<uint8_t> sortedIndices;
        std::generate_n(std::back_inserter(sortedIndices), lchColors.size(), [i = 0]() mutable
                        { return i++; });
        constexpr float epsilon = 0.1F;
        std::sort(sortedIndices.begin(), sortedIndices.end(), [lchColors](auto ia, auto ib)
                  {
                  const auto &ca = lchColors.at(ia);
                  const auto &cb = lchColors.at(ib);
                  // use closest hue distance to make hue wrap around
                  constexpr float OneOver360 = 1.0 / 360.0;
                  float distH0 = 2.0F * std::abs((ca.H() * OneOver360) - (cb.H() * OneOver360));
                  float distH1 = 2.0F - distH0;
                  float distH = distH0 < distH1 ? distH0 : distH1;
                  auto distC = std::abs(cb.C() - ca.C());
                  auto distL = std::abs(cb.L() - ca.L());
                  return (distH > epsilon && distC > epsilon && distL > epsilon) ||
                         (distH < epsilon && distC > epsilon && distL > epsilon) ||
                         (distH < epsilon && distC < epsilon && distL > epsilon); });
        // insert colors / indices successively at optimal positions
        std::vector<uint8_t> currentIndices(1, sortedIndices.front());
        for (uint32_t i = 1; i < sortedIndices.size(); i++)
        {
            currentIndices = insertIndexOptimal(currentIndices, distancesSqrMap, sortedIndices.at(i));
        }
        return currentIndices;
    }
}

#pragma once

#include "exception.h"

#include <array>
#include <vector>

namespace Color
{

    /// @brief Calculate square of distance between colors (scalar product)
    /// @return Returns average color distance in [0,1]
    template <typename T>
    static auto distance(const std::vector<T> &colors0, const std::vector<T> &colors1) -> float
    {
        REQUIRE(colors0.size() == colors1.size(), std::runtime_error, "Data size must be the same");
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
        {
            dist += T::distance(*c0It, *c1It);
        }
        return static_cast<float>(dist / colors0.size());
    }

    /// @brief Calculate square of distance between colors (scalar product)
    /// @return Returns average block color distance in [0,1]
    template <typename T, std::size_t N>
    static auto distance(const std::array<T, N> &colors0, const std::array<T, N> &colors1) -> float
    {
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
        {
            dist += T::distance(*c0It, *c1It);
        }
        return static_cast<float>(dist / N);
    }

    /// @brief Calculate square of distance between colors (scalar product) and if there are are outliers above a threshold
    /// @return Returns (true if all colors below threshold, average color distance in [0,1])
    template <typename T, std::size_t N>
    static auto distanceBelowThreshold(const std::array<T, N> &colors0, const std::array<T, N> &colors1, float threshold) -> std::pair<bool, float>
    {
        bool belowThreshold = true;
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
        {
            auto colorDist = T::distance(*c0It, *c1It);
            belowThreshold = belowThreshold ? colorDist < threshold : belowThreshold;
            dist += colorDist;
        }
        return {belowThreshold, static_cast<float>(dist / N)};
    }

}

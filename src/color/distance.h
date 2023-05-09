#pragma once

#include <array>

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

}

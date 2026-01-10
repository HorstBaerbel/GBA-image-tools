#pragma once

#include "exception.h"
#include "rgbf.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Color
{
    /// @brief Convert sRGB color to linear color
    /// @param color Input sRGB color
    /// @return Converted linear color
    /// See: https://en.wikipedia.org/wiki/SRGB#Transfer_function_(%22gamma%22)
    auto srgbToLinear(const RGBf &color) -> RGBf;

    /// @brief Convert sRGB color to linear color
    /// @param color Input sRGB color
    /// @return Converted linear color
    /// See: https://en.wikipedia.org/wiki/SRGB#Transfer_function_(%22gamma%22)
    template <typename T_IN>
    auto srgbToLinear(const T_IN &color) -> RGBf
    {
        return srgbToLinear(convertTo<RGBf>(color));
    }

    /// @brief Convert linear color to sRGB color
    /// @param color Input linear color
    /// @return Converted sRGB color
    /// See: https://en.wikipedia.org/wiki/SRGB#Transfer_function_(%22gamma%22)
    auto linearToSrgb(const RGBf &color) -> RGBf;

    /// @brief Convert linear color to sRGB color
    /// @param color Input linear color
    /// @return Converted sRGB color
    /// See: https://en.wikipedia.org/wiki/SRGB#Transfer_function_(%22gamma%22)
    template <typename T_OUT>
    auto linearToSrgb(const RGBf &color) -> T_OUT
    {
        return convertTo<T_OUT>(linearToSrgb(color));
    }

    /// @brief Convert sRGB colors to linear colors
    /// @param color Input sRGB colors
    /// @return Converted linear colors
    /// See: https://en.wikipedia.org/wiki/SRGB#Transfer_function_(%22gamma%22)
    template <typename T_IN>
    static auto srgbToLinear(const std::vector<T_IN> &colors) -> std::vector<RGBf>
    {
        std::vector<RGBf> result(colors.size());
        std::transform(colors.cbegin(), colors.cend(), result.begin(), [](const auto &c)
                       { return srgbToLinear(convertTo<RGBf>(c)); });
        return result;
    }

    /// @brief Convert linear colors to sRGB colors
    /// @param color Input linear colors
    /// @return Converted sRGB colors
    /// See: https://en.wikipedia.org/wiki/SRGB#Transfer_function_(%22gamma%22)
    template <typename T_OUT>
    static auto srgbToLinear(const std::vector<RGBf> &colors) -> std::vector<T_OUT>
    {
        std::vector<T_OUT> result(colors.size());
        std::transform(colors.cbegin(), colors.cend(), result.begin(), [](const auto &c)
                       { return convertTo<T_OUT>(linearToSrgb(c)); });
        return result;
    }
}
#pragma once

#include "exception.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Color
{

    /// @brief Convert one color format to another with clamping
    /// @tparam T_IN Input color type
    /// @tparam T_OUT Output color type
    /// @param color Input color
    /// @return Converted color
    template <typename T_OUT, typename T_IN>
    auto convertTo(const T_IN &color) -> T_OUT;

    /// @brief Convert one color format to another with clamping
    /// @tparam T_IN Input color type
    /// @tparam T_OUT Output color type
    /// @param colors Input colors
    /// @return Converted colors
    template <typename T_OUT, typename T_IN, std::size_t N>
    static auto convertTo(const std::array<T_IN, N> &colors) -> std::array<T_OUT, N>
    {
        if constexpr (std::is_same<T_IN, T_OUT>::value)
        {
            return colors;
        }
        std::array<T_OUT, N> result;
        std::transform(colors.cbegin(), colors.cend(), result.begin(), [](const auto &c)
                       { return convertTo<T_OUT>(c); });
        return result;
    }

    /// @brief Convert one color format to another with clamping
    /// @tparam T_IN Input color type
    /// @tparam T_OUT Output color type
    /// @param colors Input colors
    /// @return Converted colors
    template <typename T_OUT, typename T_IN>
    static auto convertTo(const std::vector<T_IN> &colors) -> std::vector<T_OUT>
    {
        if constexpr (std::is_same<T_IN, T_OUT>::value)
        {
            return colors;
        }
        std::vector<T_OUT> result(colors.size());
        std::transform(colors.cbegin(), colors.cend(), result.begin(), [](const auto &c)
                       { return convertTo<T_OUT>(c); });
        return result;
    }

    /// @brief Convert data in one color format to another with clamping
    /// @tparam T_IN Input color type
    /// @tparam T_OUT Output color type
    /// @param data Input color data
    /// @return Converted colors
    template <typename T_OUT, typename T_IN>
    static auto convertTo(const std::vector<uint8_t> &data) -> std::vector<T_OUT>
    {
        // convert raw data to correct input pixel format
        REQUIRE(data.size() % sizeof(typename T_IN::pixel_type) == 0, std::runtime_error, "Bad input data");
        std::vector<T_IN> colors(data.size() / sizeof(typename T_IN::pixel_type));
        std::memcpy(colors.data(), data.data(), data.size());
        if constexpr (std::is_same<T_IN, T_OUT>::value)
        {
            return colors;
        }
        return convertTo<T_OUT>(colors);
    }

    /// @brief Swap red and blue component in color
    /// @tparam T Color type
    /// @param color Input color
    /// @return Converted color
    template <typename T>
    static auto swapToBGR(const std::vector<T> &colors) -> std::vector<T>
    {
        std::vector<T> result(colors.size());
        std::transform(colors.cbegin(), colors.cend(), result.begin(), [](const auto &c)
                       { return c.swapToBGR(); });
        return result;
    }
}

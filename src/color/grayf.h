#pragma once

#include "colorformat.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace Color
{

    /// @brief Linear floating point grayscale intensity in range [0,1]
    class Grayf
    {
    public:
        using pixel_type = float; // pixel value type
        using value_type = float; // color channel value type
        static constexpr Color::Format ColorFormat = Format::Grayf;
        static constexpr uint32_t Channels = 1;

        Grayf() : v(0.0F) {}
        Grayf(float I) : v(I) {}

        inline auto I() const -> const value_type & { return v; }
        inline auto I() -> value_type & { return v; }

        inline auto operator[](std::size_t /*pos*/) const -> value_type { return v; }
        inline auto operator[](std::size_t /*pos*/) -> value_type & { return v; }

        /// @brief Return raw intensity value
        inline operator float() const { return v; }

        static constexpr std::array<value_type, 1> Min{0.0F};
        static constexpr std::array<value_type, 1> Max{1.0F};

        /// @brief Round and clamp RGB values to grid positions. The values themselves will stay in [0,1]
        /// Rounding 0.1 to 31 will result in 0.097 -> (int ((x * 31) + 0.5)) / 31
        /// @param color Input color
        /// @param gridMax Max. grid position. Grid min. will always be 0
        template <typename T>
        static auto roundTo(const Grayf &color, const std::array<T, 1> &gridMax) -> Grayf
        {
            // scale to grid
            float I = color * gridMax[0];
            // clamp to [0, gridMax]
            I = I < 0.0F ? 0.0F : (I > gridMax[0] ? gridMax[0] : I);
            // round to grid point
            I = std::trunc(I + 0.5F);
            // convert to result
            I /= gridMax[0];
            return Grayf(I);
        }

        /// @brief Calculate square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const Grayf &color0, const Grayf &color1) -> float;

    private:
        float v;
    };

}

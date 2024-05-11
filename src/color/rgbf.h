#pragma once

#include "colorformat.h"

#include <Eigen/Eigen>
#include <Eigen/Core>

#include <array>
#include <cstdint>

namespace Color
{

    /// @brief Linear floating point RGB color in range [0,1]
    class RGBf : public Eigen::Vector3f
    {
    public:
        using pixel_type = Eigen::Vector3f; // pixel value type
        using value_type = float;           // color channel value type
        static constexpr Color::Format ColorFormat = Format::RGBf;
        static constexpr uint32_t Channels = pixel_type::RowsAtCompileTime;

        RGBf() : pixel_type(0.0F, 0.0F, 0.0F) {}
        template <class... Types>
        RGBf(const Eigen::CwiseBinaryOp<Types...> &op) : pixel_type(op.matrix()) {}
        RGBf(const std::array<value_type, 3> &other) : pixel_type(other[0], other[1], other[2]) {}
        RGBf(float R, float G, float B) : pixel_type(R, G, B) {}

        inline auto R() const -> const value_type & { return x(); }
        inline auto R() -> value_type & { return x(); }
        inline auto G() const -> const value_type & { return y(); }
        inline auto G() -> value_type & { return y(); }
        inline auto B() const -> const value_type & { return z(); }
        inline auto B() -> value_type & { return z(); }

        static constexpr std::array<value_type, 3> Min{0.0F, 0.0F, 0.0F};
        static constexpr std::array<value_type, 3> Max{1.0F, 1.0F, 1.0F};

        /// @brief Return swapped red and blue color channel
        auto swappedRB() const -> RGBf;

        /// @brief Round and clamp RGB values to grid positions. The values themselves will stay in [0,1]
        /// Rounding (0.1,0.5,0.9) to (31,31,31) will result in (0.097,0.516,0.903) -> (int ((x * 31) + 0.5)) / 31
        /// @param color Input color
        /// @param gridMax Max. grid position. Grid min. will always be (0,0,0)
        template <typename T>
        static auto roundTo(const RGBf &color, const std::array<T, 3> &gridMax) -> RGBf
        {
            // scale to grid
            float R = color.R() * gridMax[0];
            float G = color.G() * gridMax[1];
            float B = color.B() * gridMax[2];
            // clamp to [0, gridMax]
            R = R < 0.0F ? 0.0F : (R > gridMax[0] ? gridMax[0] : R);
            G = G < 0.0F ? 0.0F : (G > gridMax[1] ? gridMax[1] : G);
            B = B < 0.0F ? 0.0F : (B > gridMax[2] ? gridMax[2] : B);
            // round to grid point
            R = std::trunc(R + 0.5F);
            G = std::trunc(G + 0.5F);
            B = std::trunc(B + 0.5F);
            // convert to result
            R /= gridMax[0];
            G /= gridMax[1];
            B /= gridMax[2];
            return RGBf(R, G, B);
        }

        /// @brief Calculate mean squared error between colors using simple metric
        /// @return Returns a value in [0,1]
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        static auto mse(const RGBf &color0, const RGBf &color1) -> float;
    };

}

#pragma once

#include "colorformat.h"

#include <Eigen/Core>
#include <array>
#include <cstdint>

namespace Color
{

    /// @brief Linear floating point RGB color in range [0,1]
    class RGBf : public Eigen::Vector3f
    {
    public:
        static constexpr Color::Format ColorFormat = Format::RGBf;
        using pixel_type = Eigen::Vector3f; // pixel value type
        using value_type = float;           // color channel value type

        RGBf() = default;
        RGBf(const Eigen::Vector3f &other) : Eigen::Vector3f(other) {}
        template <class... Types>
        RGBf(const Eigen::CwiseBinaryOp<Types...> &op) : Eigen::Vector3f(op.matrix()) {}
        RGBf(const std::initializer_list<value_type> &other) : Eigen::Vector3f({other}) {}
        RGBf(float R, float G, float B) : Eigen::Vector3f(R, G, B) {}

        inline auto R() const -> const value_type & { return x(); }
        inline auto R() -> value_type & { return x(); }
        inline auto G() const -> const value_type & { return y(); }
        inline auto G() -> value_type & { return y(); }
        inline auto B() const -> const value_type & { return z(); }
        inline auto B() -> value_type & { return z(); }

        inline auto raw() const -> pixel_type { return *this; }

        static constexpr std::array<value_type, 3> Min{0.0F, 0.0F, 0.0F};
        static constexpr std::array<value_type, 3> Max{1.0F, 1.0F, 1.0F};

        /// @brief Return swapped red and blue color channel
        auto swappedRB() const -> RGBf;

        /// @brief Round and clamp RGB values to grid positions. The values themselves will stay in [0,1]
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

        /// @brief Calculate square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const RGBf &color0, const RGBf &color1) -> float;
    };

}

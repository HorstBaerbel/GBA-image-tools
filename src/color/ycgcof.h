#pragma once

#include "colorformat.h"

#include <Eigen/Core>
#include <array>
#include <cstdint>

namespace Color
{

    /// @brief Linear floating point YCgCoR color in range
    /// Y [0,1] Luma
    /// Cg [-1,1] Chroma green
    /// Co [-1,1] Chroma orange
    /// See: https://en.wikipedia.org/wiki/YCoCg#The_lifting-based_YCoCg-R_variation
    class YCgCoRf : public Eigen::Vector3f
    {
    public:
        static constexpr Color::Format ColorFormat = Format::YCgCoRf;
        using pixel_type = Eigen::Vector3f; // pixel value type
        using value_type = float;           // color channel value type

        YCgCoRf() : Eigen::Vector3f() {}
        YCgCoRf(const Eigen::Vector3f &other) : Eigen::Vector3f(other) {}
        template <class... Types>
        YCgCoRf(const Eigen::CwiseBinaryOp<Types...> &op) : Eigen::Vector3f(op.matrix()) {}
        YCgCoRf(const std::initializer_list<value_type> &other) : Eigen::Vector3f({other}) {}
        YCgCoRf(float Y, float Cg, float Co) : Eigen::Vector3f(Y, Cg, Co) {}

        inline auto Y() const -> const value_type & { return x(); }
        inline auto Y() -> value_type & { return x(); }
        inline auto Cg() const -> const value_type & { return y(); }
        inline auto Cg() -> value_type & { return y(); }
        inline auto Co() const -> const value_type & { return z(); }
        inline auto Co() -> value_type & { return z(); }

        inline auto raw() const -> pixel_type { return *this; }

        static constexpr std::array<value_type, 3> Min{0.0F, -1.0F, -1.0F};
        static constexpr std::array<value_type, 3> Max{1.0F, 1.0F, 1.0F};

        /// @brief Return color with all components normalized to [0,1]. This is NOT a conversion to RGB!
        auto normalized() const -> YCgCoRf;

        /// @brief Round and clamp YCgCoR values to grid positions. The values themselves will stay in their respective ranges
        template <typename T>
        static auto roundTo(const YCgCoRf &color, const std::array<T, 3> &gridMax) -> YCgCoRf
        {
            // convert to float RGB
            float tmp = color.Y() - color.Cg() / 2.0F;
            float G = color.Cg() + tmp;
            float B = tmp - color.Co() / 2.0F;
            float R = B + color.Co();
            // scale to grid
            R *= gridMax[0];
            G *= gridMax[1];
            B *= gridMax[2];
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
            float Co = R - B;
            tmp = B + Co / 2.0F;
            float Cg = G - tmp;
            float Y = tmp + Cg / 2.0F;
            return YCgCoRf(Y, Cg, Co);
        }

        /// @brief Calculate square of distance between colors (scalar product)
        /// @return Returns color distance in [0,1]
        static auto distance(const YCgCoRf &color0, const YCgCoRf &color1) -> float;
    };

}

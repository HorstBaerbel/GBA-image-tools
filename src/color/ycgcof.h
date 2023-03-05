#pragma once

#include <Eigen/Core>
#include <array>
#include <cstdint>

namespace Color
{

    /// @brief Floating point YCgCoR color in range
    /// Y [0,1] Luma
    /// Cg [-1,1] Chroma green
    /// Co [-1,1] Chroma orange
    // See: https://en.wikipedia.org/wiki/YCoCg#The_lifting-based_YCoCg-R_variation
    class YCgCoRf : public Eigen::Vector3f
    {
    public:
        YCgCoRf() : Eigen::Vector3f() {}
        YCgCoRf(const Eigen::Vector3f &other) : Eigen::Vector3f(other) {}
        template <class... Types>
        YCgCoRf(const Eigen::CwiseBinaryOp<Types...> &op) : Eigen::Vector3f(op.matrix()) {}
        YCgCoRf(const std::initializer_list<float> &other) : Eigen::Vector3f({other}) {}
        YCgCoRf(float Y, float Cg, float Co) : Eigen::Vector3f(Y, Cg, Co) {}

        inline auto Y() const -> const float & { return x(); }
        inline auto Y() -> float & { return x(); }
        inline auto Cg() const -> const float & { return y(); }
        inline auto Cg() -> float & { return y(); }
        inline auto Co() const -> const float & { return z(); }
        inline auto Co() -> float & { return z(); }

        static const YCgCoRf Min;
        static const YCgCoRf Max;

        /// @brief Return color with all components normalized to [0,1]
        auto normalized() const -> YCgCoRf;

        /// @brief YCgCoR color from RGB values in [0,1]
        static auto fromRGB(float R, float G, float B) -> YCgCoRf;

        /// @brief YCgCoR color from raw 24-bit RGB888 data
        static auto fromRGB888(const uint8_t *rgb888) -> YCgCoRf;

        /// @brief YCgCoR color from raw 32-bit XRGB888 data
        static auto fromXRGB888(uint32_t xrgb888) -> YCgCoRf;

        /// @brief YCgCoR color from raw RGB555 uint16_t
        static auto fromRGB555(uint16_t color) -> YCgCoRf;

        /// @brief Convert color to raw RGB555 uint16_t by truncating and clamping
        auto toRGB555() const -> uint16_t;

        /// @brief Convert colors to raw RGB555 uint16_t by truncating and clamping
        template <std::size_t N>
        static auto toRGB555(const std::array<YCgCoRf, N> &colors) -> std::array<uint16_t, N>
        {
            std::array<uint16_t, N> result;
            std::transform(colors.cbegin(), colors.cend(), result.begin(), [](const auto &c)
                           { return c.toRGB555(); });
            return result;
        }

        /// @brief Round and clamp YCgCoR values to RGB555 grid positions. The values themselves will stay in their ranges
        static auto roundToRGB555(const YCgCoRf &color) -> YCgCoRf;

        /// @brief Calculate square of distance between colors (scalar product)
        /// @return Returns color distance in [0,1]
        static auto distance(const YCgCoRf &color0, const YCgCoRf &color1) -> float;

        /// @brief Calculate square of distance between colors (scalar product)
        /// @return Returns block color distance in [0,1]
        template <std::size_t N>
        friend auto distance(const std::array<YCgCoRf, N> &colors0, const std::array<YCgCoRf, N> &colors1) -> float
        {
            float dist = 0.0F;
            for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
            {
                dist += distance(*c0It, *c1It);
            }
            return dist / (N * N);
        }

        /// @brief Calculate square of distance between colors (scalar product) and if there are are outliers above a threshold
        /// @return Returns (allColorsBelowThreshold?, block color distance in [0,1])
        template <std::size_t N>
        friend auto distanceBelowThreshold(const std::array<YCgCoRf, N> &colors0, const std::array<YCgCoRf, N> &colors1, float threshold) -> std::pair<bool, float>
        {
            bool belowThreshold = true;
            float dist = 0.0F;
            for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
            {
                auto colorDist = distance(*c0It, *c1It);
                belowThreshold = belowThreshold ? colorDist < threshold : belowThreshold;
                dist += colorDist;
            }
            return {belowThreshold, dist / (N * N)};
        }
    };

}

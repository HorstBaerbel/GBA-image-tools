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
    class YCgCoRd : public Eigen::Vector3d
    {
    public:
        YCgCoRd() : Eigen::Vector3d() {}
        YCgCoRd(const Eigen::Vector3d &other) : Eigen::Vector3d(other) {}
        template <class... Types>
        YCgCoRd(const Eigen::CwiseBinaryOp<Types...> &op) : Eigen::Vector3d(op.matrix()) {}
        YCgCoRd(const std::initializer_list<double> &other) : Eigen::Vector3d({other}) {}
        YCgCoRd(double Y, double Cg, double Co) : Eigen::Vector3d(Y, Cg, Co) {}

        inline auto Y() const -> const double & { return x(); }
        inline auto Y() -> double & { return x(); }
        inline auto Cg() const -> const double & { return y(); }
        inline auto Cg() -> double & { return y(); }
        inline auto Co() const -> const double & { return z(); }
        inline auto Co() -> double & { return z(); }

        /// @brief Return color with all components normalized to [0,1]
        auto normalized() const -> YCgCoRd;

        /// @brief YCgCoR color from RGB values in [0,1]
        static auto fromRGB(double R, double G, double B) -> YCgCoRd;

        /// @brief YCgCoR color from raw 24-bit RGB888 data
        static auto fromRGB888(const uint8_t *rgb888) -> YCgCoRd;

        /// @brief YCgCoR color from raw 32-bit XRGB888 data
        static auto fromXRGB888(uint32_t xrgb888) -> YCgCoRd;

        /// @brief YCgCoR color from raw RGB555 uint16_t
        static auto fromRGB555(uint16_t color) -> YCgCoRd;

        /// @brief Convert color to raw RGB555 uint16_t by truncating and clamping
        auto toRGB555() const -> uint16_t;

        /// @brief Convert colors to raw RGB555 uint16_t by truncating and clamping
        template <std::size_t N>
        static auto toRGB555(const std::array<YCgCoRd, N> &colors) -> std::array<uint16_t, N>
        {
            std::array<uint16_t, N> result;
            std::transform(colors.cbegin(), colors.cend(), result.begin(), [](const auto &c)
                           { return c.toRGB555(); });
            return result;
        }

        /// @brief Round and clamp YCgCoR values to RGB555 grid positions. The values themselves will stay in their ranges
        static auto roundToRGB555(const YCgCoRd &color) -> YCgCoRd;

        /// @brief Calculate square of distance between colors (scalar product)
        /// @return Returns color distance in [0,1]
        static auto distance(const YCgCoRd &color0, const YCgCoRd &color1) -> double;

        /// @brief Calculate square of distance between colors (scalar product)
        /// @return Returns block color distance in [0,1]
        template <std::size_t N>
        static auto distance(const std::array<YCgCoRd, N> &colors0, const std::array<YCgCoRd, N> &colors1) -> double
        {
            double dist = 0.0;
            for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
            {
                dist += distance(*c0It, *c1It);
            }
            return dist / N;
        }

        /// @brief Calculate square of distance between colors (scalar product) and if there are are outliers above a threshold
        /// @return Returns (allColorsBelowThreshold?, block color distance in [0,1])
        template <std::size_t N>
        static auto distanceBelowThreshold(const std::array<YCgCoRd, N> &colors0, const std::array<YCgCoRd, N> &colors1, double threshold) -> std::pair<bool, double>
        {
            bool belowThreshold = true;
            double dist = 0.0;
            for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend() && c1It != colors1.cend(); ++c0It, ++c1It)
            {
                auto colorDist = distance(*c0It, *c1It);
                belowThreshold = belowThreshold ? colorDist < threshold : belowThreshold;
                dist += colorDist;
            }
            return {belowThreshold, dist / N};
        }
    };

}

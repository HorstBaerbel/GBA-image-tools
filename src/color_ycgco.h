#pragma once

#include <Eigen/Core>
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

        /// @brief YCgCoR color from raw 24bit RGB888 data
        static auto fromRGB888(const uint8_t *rgb888) -> YCgCoRd;

        /// @brief YCgCoR color from raw RGB555 uint16_t
        static auto fromRGB555(uint16_t color) -> YCgCoRd;

        /// @brief Convert color to raw RGB555 uint16_t by truncating and clamping
        auto toRGB555() const -> uint16_t;

        /// @brief Round and clamp YCgCoR values to RGB555 grid positions. The values themselves will stay in their ranges
        static auto roundToRGB555(const YCgCoRd &color) -> YCgCoRd;

        /// @brief Calculate square of distance between colors (scalar product)
        /// @return Returns a value in [0,1]
        static auto distance(const YCgCoRd &color0, const YCgCoRd &color1) -> double;

        /// @brief Calculate square of distance between colors (scalar product)
        /// @return Returns a value in [0,1]
        static auto distance(const std::array<YCgCoRd, 16> &colors0, const std::array<YCgCoRd, 16> &colors1) -> double;

        /// @brief Calculate distance between DCT-transformed blocks.
        /// @return Returns a value in [0,1]
        static auto dctDistance(const std::array<YCgCoRd, 16> &colors0, const std::array<YCgCoRd, 16> &colors1) -> double;
    };

}

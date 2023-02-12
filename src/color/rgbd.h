#pragma once

#include <Eigen/Core>
#include <cstdint>

namespace Color
{

    /// @brief Floating point RGB color in range [0,1]
    class RGBd : public Eigen::Vector3d
    {
    public:
        RGBd() : Eigen::Vector3d() {}
        RGBd(const Eigen::Vector3d &other) : Eigen::Vector3d(other) {}
        template <class... Types>
        RGBd(const Eigen::CwiseBinaryOp<Types...> &op) : Eigen::Vector3d(op.matrix()) {}
        RGBd(const std::initializer_list<double> &other) : Eigen::Vector3d({other}) {}
        RGBd(double R, double G, double B) : Eigen::Vector3d(R, G, B) {}

        inline auto R() const -> const double & { return x(); }
        inline auto R() -> double & { return x(); }
        inline auto G() const -> const double & { return y(); }
        inline auto G() -> double & { return y(); }
        inline auto B() const -> const double & { return z(); }
        inline auto B() -> double & { return z(); }

        /// @brief RGB color from raw 24-bit RGB888 data
        static auto fromRGB888(const uint8_t *rgb888) -> RGBd;

        /// @brief RGB color from raw 32-bit XRGB888 data
        static auto fromXRGB888(const uint32_t *xrgb888) -> RGBd;

        /// @brief RGB color from raw RGB555 uint16_t
        static auto fromRGB555(uint16_t color) -> RGBd;

        /// @brief Convert color to raw RGB555 uint16_t by truncating and clamping
        auto toRGB555() const -> uint16_t;

        /// @brief Round and clamp RGB values to RGB555 grid positions. The values themselves will stay in [0,1]
        static auto roundToRGB555(const RGBd &color) -> RGBd;

        /// @brief Calculate square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const RGBd &color0, const RGBd &color1) -> double;

        /// @brief Calculate sum of square of perceived distance between colors
        /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
        /// @return Returns a value in [0,1]
        static auto distance(const std::array<RGBd, 16> &colors0, const std::array<RGBd, 16> &colors1) -> double;
    };

}

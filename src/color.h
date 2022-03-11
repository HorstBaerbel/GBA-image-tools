#pragma once

#include <Eigen/Core>
#include <cstdint>

namespace Color
{

    struct YCgCoRd
    {
        double Y = 0;
        double Cg = 0;
        double Co = 0;

        static YCgCoRd fromRGB888(const uint8_t *rgb888);
    }; // See: https://en.wikipedia.org/wiki/YCoCg

    /// @brief Floating point RGB color in range [0,1]
    using RGBd = Eigen::Vector3d;

    /// @brief RGB color from raw RGB555 uint16_t
    RGBd fromRGB555(uint16_t color);

    /// @brief Convert color to raw RGB555 uint16_t by truncating and clamping
    uint16_t toRGB555(const RGBd &color);

    /// @brief Round and clamp RGB values to RGB555 grid positions. The values themselves will stay in [0,1]
    RGBd roundToRGB555(const RGBd &color);

    /// @brief Calculate square of perceived distance between colors
    /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
    /// @return Returns a value in [0,9]
    double distance(const RGBd &color0, const RGBd &color1);

    /// @brief Calculate square of perceived distance between colors
    /// See: https://stackoverflow.com/a/40950076 and https://www.compuphase.com/cmetric.htm
    /// @return Returns a value in [0,9]
    double distance(const std::array<RGBd, 16> &colors0, const std::array<RGBd, 16> &colors1);

    /// @brief Fit a line through colors colors passed using SVD
    /// Found here: https://stackoverflow.com/questions/40589802/eigen-best-fit-of-a-plane-to-n-points
    /// See also: https://zalo.github.io/blog/line-fitting/
    /// See also: https://stackoverflow.com/questions/39370370/eigen-and-svd-to-find-best-fitting-plane-given-a-set-of-points
    /// See also: https://gist.github.com/ialhashim/0a2554076a6cf32831ca
    /// @return Returns line (origin, axis)
    std::pair<RGBd, RGBd> lineFit(const std::array<Color::RGBd, 16> &colors);

}

#include "rgbd.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    auto RGBd::fromRGB888(const uint8_t *rgb888) -> RGBd
    {
        RGBd result;
        result.R() = static_cast<double>(rgb888[0]) / 255.0;
        result.G() = static_cast<double>(rgb888[1]) / 255.0;
        result.B() = static_cast<double>(rgb888[2]) / 255.0;
        return result;
    }

    auto RGBd::fromXRGB888(const uint32_t *xrgb888) -> RGBd
    {
        RGBd result;
        uint32_t color = *xrgb888;
        result.R() = static_cast<double>(color & 0xFF) / 255.0;
        result.G() = static_cast<double>((color >> 8) & 0xFF) / 255.0;
        result.B() = static_cast<double>((color >> 16) & 0xFF) / 255.0;
        return result;
    }

    auto RGBd::fromRGB555(uint16_t color) -> RGBd
    {
        RGBd result;
        result.R() = static_cast<double>((color & 0x7C00) >> 10) / 31.0;
        result.G() = static_cast<double>((color & 0x3E0) >> 5) / 31.0;
        result.B() = static_cast<double>(color & 0x1F) / 31.0;
        return result;
    }

    auto RGBd::toRGB555() const -> uint16_t
    {
        // bring into range
        auto cr = R() * 31.0;
        auto cg = G() * 31.0;
        auto cb = B() * 31.0;
        // clamp to [0,31]
        cr = cr < 0.0 ? 0.0 : (cr > 31.0 ? 31.0 : cr);
        cg = cg < 0.0 ? 0.0 : (cg > 31.0 ? 31.0 : cg);
        cb = cb < 0.0 ? 0.0 : (cb > 31.0 ? 31.0 : cb);
        // convert to RGB555
        return (static_cast<uint16_t>(cr) << 10) | (static_cast<uint16_t>(cg) << 5) | static_cast<uint16_t>(cb);
    }

    auto RGBd::roundToRGB555(const RGBd &color) -> RGBd
    {
        // bring into range
        auto cr = color.R() * 31.0;
        auto cg = color.G() * 31.0;
        auto cb = color.B() * 31.0;
        // clamp to [0, GRID_MAX]
        cr = cr < 0.0 ? 0.0 : (cr > 31.0 ? 31.0 : cr);
        cg = cg < 0.0 ? 0.0 : (cg > 31.0 ? 31.0 : cg);
        cb = cb < 0.0 ? 0.0 : (cb > 31.0 ? 31.0 : cb);
        // round to grid point
        cr = std::trunc(cr + 0.5);
        cg = std::trunc(cg + 0.5);
        cb = std::trunc(cb + 0.5);
        RGBd result;
        result.R() = cr / 31.0;
        result.G() = cg / 31.0;
        result.B() = cb / 31.0;
        return result;
    }

    auto RGBd::distance(const RGBd &color0, const RGBd &color1) -> double
    {
        if (color0 == color1)
        {
            return 0.0;
        }
        double ra = color0.R();
        double rb = color1.R();
        double r = 0.5 * (ra + rb);
        double dR = ra - rb;
        double dG = color0.G() - color1.G();
        double dB = color0.B() - color1.B();
        return ((2.0 + r) * dR * dR + 4.0 * dG * dG + (3.0 - r) * dB * dB) / 9.0;
    } // max:  (2 + 0.5) *  1 *  1 + 4   *  1 *  1 + (3 - 0.5) *  1 *  1 = 2.5 + 4 + 2.5 = 9 / 9 = 1

    auto RGBd::distance(const std::array<RGBd, 16> &colors0, const std::array<RGBd, 16> &colors1) -> double
    {
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend(); ++c0It, ++c1It)
        {
            dist += distance(*c0It, *c1It);
        }
        return dist / 16.0;
    }

}
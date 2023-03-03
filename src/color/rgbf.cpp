#include "rgbf.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    const RGBf RGBf::Min{0.0F, 0.0F, 0.0F};
    const RGBf RGBf::Max{1.0F, 1.0F, 1.0F};

    auto RGBf::fromRGB888(const uint8_t *rgb888) -> RGBf
    {
        RGBf result;
        result.R() = static_cast<float>(rgb888[0]) / 255.0F;
        result.G() = static_cast<float>(rgb888[1]) / 255.0F;
        result.B() = static_cast<float>(rgb888[2]) / 255.0F;
        return result;
    }

    auto RGBf::fromXRGB888(uint32_t xrgb888) -> RGBf
    {
        RGBf result;
        result.R() = static_cast<float>(xrgb888 & 0xFF) / 255.0F;
        result.G() = static_cast<float>((xrgb888 >> 8) & 0xFF) / 255.0F;
        result.B() = static_cast<float>((xrgb888 >> 16) & 0xFF) / 255.0F;
        return result;
    }

    auto RGBf::fromRGB555(uint16_t color) -> RGBf
    {
        RGBf result;
        result.R() = static_cast<float>((color & 0x7C00) >> 10) / 31.0F;
        result.G() = static_cast<float>((color & 0x3E0) >> 5) / 31.0F;
        result.B() = static_cast<float>(color & 0x1F) / 31.0F;
        return result;
    }

    auto RGBf::toRGB555() const -> uint16_t
    {
        // bring into range
        auto cr = R() * 31.0F;
        auto cg = G() * 31.0F;
        auto cb = B() * 31.0F;
        // clamp to [0,31]
        cr = cr < 0.0F ? 0.0F : (cr > 31.0F ? 31.0F : cr);
        cg = cg < 0.0F ? 0.0F : (cg > 31.0F ? 31.0F : cg);
        cb = cb < 0.0F ? 0.0F : (cb > 31.0F ? 31.0F : cb);
        // convert to RGB555
        return (static_cast<uint16_t>(cr) << 10) | (static_cast<uint16_t>(cg) << 5) | static_cast<uint16_t>(cb);
    }

    auto RGBf::roundToRGB555(const RGBf &color) -> RGBf
    {
        // bring into range
        auto cr = color.R() * 31.0F;
        auto cg = color.G() * 31.0F;
        auto cb = color.B() * 31.0F;
        // clamp to [0, GRID_MAX]
        cr = cr < 0.0F ? 0.0F : (cr > 31.0F ? 31.0F : cr);
        cg = cg < 0.0F ? 0.0F : (cg > 31.0F ? 31.0F : cg);
        cb = cb < 0.0F ? 0.0F : (cb > 31.0F ? 31.0F : cb);
        // round to grid point
        cr = std::trunc(cr + 0.5F);
        cg = std::trunc(cg + 0.5F);
        cb = std::trunc(cb + 0.5F);
        RGBf result;
        result.R() = cr / 31.0F;
        result.G() = cg / 31.0F;
        result.B() = cb / 31.0F;
        return result;
    }

    auto RGBf::distance(const RGBf &color0, const RGBf &color1) -> float
    {
        if (color0 == color1)
        {
            return 0.0F;
        }
        float ra = color0.R();
        float rb = color1.R();
        float rMean = 0.5F * (ra + rb);
        float dR = ra - rb;
        float dG = color0.G() - color1.G();
        float dB = color0.B() - color1.B();
        return ((2.0F + rMean) * dR * dR + 4.0F * dG * dG + (3.0F - rMean) * dB * dB) / 9.0F;
    } // max:   (2    +     1) *  1 *  1 + 4    *  1 *  1 + (3    -     1) *  1 *  1 = 3 + 4 + 2 = 9 / 9 = 1

    auto RGBf::distance(const std::array<RGBf, 16> &colors0, const std::array<RGBf, 16> &colors1) -> float
    {
        float dist = 0.0F;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.cend(); ++c0It, ++c1It)
        {
            dist += distance(*c0It, *c1It);
        }
        return dist / 16.0F;
    }

}
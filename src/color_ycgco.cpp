#include "color_ycgco.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    auto YCgCoRd::fromRGB888(const uint8_t *rgb888) -> YCgCoRd
    {
        double R = static_cast<double>(rgb888[0]) / 255.0;
        double G = static_cast<double>(rgb888[1]) / 255.0;
        double B = static_cast<double>(rgb888[2]) / 255.0;
        YCgCoRd result;
        result.Co() = R - B;
        double tmp = B + result.Co() / 2.0;
        result.Cg() = G - tmp;
        result.Y() = tmp + result.Cg() / 2.0;
        return result;
    }

    auto YCgCoRd::fromRGB555(uint16_t color) -> YCgCoRd
    {
        double R = static_cast<double>((color & 0x7C00) >> 10) / 31.0;
        double G = static_cast<double>((color & 0x3E0) >> 5) / 31.0;
        double B = static_cast<double>(color & 0x1F) / 31.0;
        YCgCoRd result;
        result.Co() = R - B;
        double tmp = B + result.Co() / 2.0;
        result.Cg() = G - tmp;
        result.Y() = tmp + result.Cg() / 2.0;
        return result;
    }

    auto YCgCoRd::toRGB555() const -> uint16_t
    {
        // bring into range
        double tmp = Y() - Cg() / 2.0;
        double G = Cg() + tmp;
        double B = tmp - Co() / 2.0;
        double R = B + Co();
        R *= 31.0;
        G *= 31.0;
        B *= 31.0;
        // clamp to [0,31]
        R = R < 0.0 ? 0.0 : (R > 31.0 ? 31.0 : R);
        G = G < 0.0 ? 0.0 : (G > 31.0 ? 31.0 : G);
        B = B < 0.0 ? 0.0 : (B > 31.0 ? 31.0 : B);
        // convert to RGB555
        return (static_cast<uint16_t>(R) << 10) | (static_cast<uint16_t>(G) << 5) | static_cast<uint16_t>(B);
    }

    auto YCgCoRd::roundToRGB555(const YCgCoRd &color) -> YCgCoRd
    {
        // convert to RGB
        double tmp = color.Y() - color.Cg() / 2.0;
        double G = color.Cg() + tmp;
        double B = tmp - color.Co() / 2.0;
        double R = B + color.Co();
        // bring into range
        R *= 31.0;
        G *= 31.0;
        B *= 31.0;
        // clamp to [0,31]
        R = R < 0.0 ? 0.0 : (R > 31.0 ? 31.0 : R);
        G = G < 0.0 ? 0.0 : (G > 31.0 ? 31.0 : G);
        B = B < 0.0 ? 0.0 : (B > 31.0 ? 31.0 : B);
        // round to grid point
        R = std::trunc(R + 0.5);
        G = std::trunc(G + 0.5);
        B = std::trunc(B + 0.5);
        // convert to result
        R /= 31.0;
        G /= 31.0;
        B /= 31.0;
        YCgCoRd result;
        result.Co() = R - B;
        tmp = B + result.Co() / 2.0;
        result.Cg() = G - tmp;
        result.Y() = tmp + result.Cg() / 2.0;
        return result;
    }

    auto YCgCoRd::distance(const YCgCoRd &color0, const YCgCoRd &color1) -> double
    {
        if (color0 == color1)
        {
            return 0.0;
        }
        auto dY = 2.0 * (color0.Y() - color1.Y()); // [0,2]
        auto dCg = color0.Cg() - color1.Cg();      // [0,2]
        auto dCo = color0.Co() - color1.Co();      // [0,2]
        return (dY * dY + dCg * dCg + dCo * dCo) / 12.0;
    } // max: (2*2 + 2*2 + 2*2) / 12 = 12 / 12 / 1

    auto YCgCoRd::distance(const std::array<YCgCoRd, 16> &colors0, const std::array<YCgCoRd, 16> &colors1) -> double
    {
        double dist = 0.0;
        for (auto c0It = colors0.cbegin(), c1It = colors1.cbegin(); c0It != colors0.end(); ++c0It, ++c1It)
        {
            dist += distance(*c0It, *c1It);
        }
        return dist / 16.0;
    }

}
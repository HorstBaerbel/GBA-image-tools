#include "ycgcof.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    const YCgCoRf YCgCoRf::Min{0.0F, -1.0F, -1.0F};
    const YCgCoRf YCgCoRf::Max{1.0F, 1.0F, 1.0F};

    auto YCgCoRf::normalized() const -> YCgCoRf
    {
        return {Y(), 0.5F * (Cg() + 1.0F), 0.5F * (Co() + 1.0F)};
    }

    auto YCgCoRf::fromRGB(float R, float G, float B) -> YCgCoRf
    {
        YCgCoRf result;
        result.Co() = R - B;
        float tmp = B + result.Co() / 2.0F;
        result.Cg() = G - tmp;
        result.Y() = tmp + result.Cg() / 2.0F;
        return result;
    }

    auto YCgCoRf::fromRGB888(const uint8_t *rgb888) -> YCgCoRf
    {
        float R = static_cast<float>(rgb888[0]) / 255.0F;
        float G = static_cast<float>(rgb888[1]) / 255.0F;
        float B = static_cast<float>(rgb888[2]) / 255.0F;
        return fromRGB(R, G, B);
    }

    auto YCgCoRf::fromXRGB888(uint32_t xrgb888) -> YCgCoRf
    {
        float R = static_cast<float>(xrgb888 & 0xFF) / 255.0F;
        float G = static_cast<float>((xrgb888 >> 8) & 0xFF) / 255.0F;
        float B = static_cast<float>((xrgb888 >> 16) & 0xFF) / 255.0F;
        return fromRGB(R, G, B);
    }

    auto YCgCoRf::fromRGB555(uint16_t color) -> YCgCoRf
    {
        float R = static_cast<float>((color & 0x7C00) >> 10) / 31.0F;
        float G = static_cast<float>((color & 0x3E0) >> 5) / 31.0F;
        float B = static_cast<float>(color & 0x1F) / 31.0F;
        return fromRGB(R, G, B);
    }

    auto YCgCoRf::toRGB555() const -> uint16_t
    {
        // bring into range
        float tmp = Y() - Cg() / 2.0F;
        float G = Cg() + tmp;
        float B = tmp - Co() / 2.0F;
        float R = B + Co();
        R *= 31.0F;
        G *= 31.0F;
        B *= 31.0F;
        // clamp to [0,31]
        R = R < 0.0F ? 0.0F : (R > 31.0F ? 31.0F : R);
        G = G < 0.0F ? 0.0F : (G > 31.0F ? 31.0F : G);
        B = B < 0.0F ? 0.0F : (B > 31.0F ? 31.0F : B);
        // convert to RGB555
        return (static_cast<uint16_t>(R) << 10) | (static_cast<uint16_t>(G) << 5) | static_cast<uint16_t>(B);
    }

    auto YCgCoRf::roundToRGB555(const YCgCoRf &color) -> YCgCoRf
    {
        // convert to RGB
        float tmp = color.Y() - color.Cg() / 2.0F;
        float G = color.Cg() + tmp;
        float B = tmp - color.Co() / 2.0F;
        float R = B + color.Co();
        // bring into range
        R *= 31.0F;
        G *= 31.0F;
        B *= 31.0F;
        // clamp to [0,31]
        R = R < 0.0F ? 0.0F : (R > 31.0F ? 31.0F : R);
        G = G < 0.0F ? 0.0F : (G > 31.0F ? 31.0F : G);
        B = B < 0.0F ? 0.0F : (B > 31.0F ? 31.0F : B);
        // round to grid point
        R = std::trunc(R + 0.5F);
        G = std::trunc(G + 0.5F);
        B = std::trunc(B + 0.5F);
        // convert to result
        R /= 31.0F;
        G /= 31.0F;
        B /= 31.0F;
        YCgCoRf result;
        result.Co() = R - B;
        tmp = B + result.Co() / 2.0F;
        result.Cg() = G - tmp;
        result.Y() = tmp + result.Cg() / 2.0F;
        return result;
    }

    auto YCgCoRf::distance(const YCgCoRf &color0, const YCgCoRf &color1) -> float
    {
        if (color0 == color1)
        {
            return 0.0F;
        }
        auto dY = color0.Y() - color1.Y();             // [0,1]
        auto dCg = 0.5F * (color0.Cg() - color1.Cg()); // [0,1]
        auto dCo = 0.5F * (color0.Co() - color1.Co()); // [0,1]
        return (2.0F * dY * dY + dCg * dCg + dCo * dCo) / 4.0F;
    } // max: (2 + 1 + 1) / 4 = 1

}
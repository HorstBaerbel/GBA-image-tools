#include "xrgb888.h"

namespace Color
{

    auto XRGB888::fromRGBf(float R, float G, float B) -> XRGB888
    {
        float rf = R * 255.0F;
        float gf = G * 255.0F;
        float bf = B * 255.0F;
        // clamp to [0,255]
        uint8_t r = rf < 0.0F ? 0 : (rf > 255.0F ? 255 : static_cast<uint8_t>(rf));
        uint8_t g = gf < 0.0F ? 0 : (gf > 255.0F ? 255 : static_cast<uint8_t>(gf));
        uint8_t b = bf < 0.0F ? 0 : (bf > 255.0F ? 255 : static_cast<uint8_t>(bf));
        return XRGB888(r, g, b);
    }

    auto fromRGBf(double R, double G, double B) -> XRGB888
    {
        double rf = R * 255.0;
        double gf = G * 255.0;
        double bf = B * 255.0;
        // clamp to [0,255]
        uint8_t r = rf < 0.0 ? 0 : (rf > 255.0 ? 255 : static_cast<uint8_t>(rf));
        uint8_t g = gf < 0.0 ? 0 : (gf > 255.0 ? 255 : static_cast<uint8_t>(gf));
        uint8_t b = bf < 0.0 ? 0 : (bf > 255.0 ? 255 : static_cast<uint8_t>(bf));
        return XRGB888(r, g, b);
    }

    auto XRGB888::distance(const XRGB888 &color0, const XRGB888 &color1) -> float
    {
        if (color0.c == color1.c)
        {
            return 0.0F;
        }
        float ra = static_cast<float>(color0.R()) / 255.0F;
        float rb = static_cast<float>(color1.R()) / 255.0F;
        float rMean = 0.5F * (ra + rb);
        float dR = ra - rb;
        float dG = static_cast<float>(color0.G()) - static_cast<float>(color1.G()) / 255.0F;
        float dB = static_cast<float>(color0.B()) - static_cast<float>(color1.B()) / 255.0F;
        return ((2.0F + rMean) * dR * dR + 4.0F * dG * dG + (3.0F - rMean) * dB * dB) / 9.0F;
    } // max:   (2    +   0.5) *  1 *  1 + 4    *  1 *  1 + (3 -      0.5) *  1 *  1 = 2.5 + 4 + 2.5 = 9 / 9 = 1

}
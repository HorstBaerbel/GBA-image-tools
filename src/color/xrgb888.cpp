#include "xrgb888.h"

#include <sstream>
#include <iomanip>

namespace Color
{

    const XRGB888 XRGB888::Min{0, 0, 0};
    const XRGB888 XRGB888::Max{255, 255, 255};

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

    auto XRGB888::fromRGBf(double R, double G, double B) -> XRGB888
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

    auto XRGB888::toHex() const -> std::string
    {
        return toHex(*this);
    }

    auto XRGB888::toHex(const XRGB888 &color) -> std::string
    {
        std::stringstream ss;
        ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(color.r) << std::setw(2) << static_cast<uint32_t>(color.g) << std::setw(2) << static_cast<uint32_t>(color.b);
        return ss.str();
    }

    auto XRGB888::distance(const XRGB888 &color0, const XRGB888 &color1) -> float
    {
        static constexpr float OneOver255 = 1.0F / 255.0F;
        if (color0.c == color1.c)
        {
            return 0.0F;
        }
        float ra = static_cast<float>(color0.R()) * OneOver255;
        float rb = static_cast<float>(color1.R()) * OneOver255;
        float rMean = 0.5F * (ra + rb);
        float dR = ra - rb;
        float dG = (static_cast<float>(color0.G()) - static_cast<float>(color1.G())) * OneOver255;
        float dB = (static_cast<float>(color0.B()) - static_cast<float>(color1.B())) * OneOver255;
        return ((2.0F + rMean) * dR * dR + 4.0F * dG * dG + (3.0F - rMean) * dB * dB) / 9.0F;
    } // max:   (2    +     1) *  1 *  1 + 4    *  1 *  1 + (3    -     1) *  1 *  1 = 3 + 4 + 2 = 9 / 9 = 1

}
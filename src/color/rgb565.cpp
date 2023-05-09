#include "rgb565.h"

namespace Color
{

    auto RGB565::swappedRB() const -> RGB565
    {
        return RGB565(v.b, v.g, v.r);
    }

    auto RGB565::distance(const RGB565 &color0, const RGB565 &color1) -> float
    {
        static constexpr float OneOver31 = 1.0F / 31.0F;
        static constexpr float OneOver63 = 1.0F / 63.0F;
        if (color0.raw() == color1.raw())
        {
            return 0.0F;
        }
        float ra = static_cast<float>(color0.R()) * OneOver31;
        float rb = static_cast<float>(color1.R()) * OneOver31;
        float rMean = 0.5F * (ra + rb);
        float dR = ra - rb;
        float dG = (static_cast<float>(color0.G()) - static_cast<float>(color1.G())) * OneOver63;
        float dB = (static_cast<float>(color0.B()) - static_cast<float>(color1.B())) * OneOver31;
        return ((2.0F + rMean) * dR * dR + 4.0F * dG * dG + (3.0F - rMean) * dB * dB) / 9.0F;
    } // max:   (2    +     1) *  1 *  1 + 4    *  1 *  1 + (3    -     1) *  1 *  1 = 3 + 4 + 2 = 9 / 9 = 1

}
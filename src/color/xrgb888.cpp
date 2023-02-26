#include "xrgb888.h"

namespace Color
{

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
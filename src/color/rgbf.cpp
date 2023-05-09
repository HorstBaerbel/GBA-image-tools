#include "rgbf.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    auto RGBf::swappedRB() const -> RGBf
    {
        return RGBf(z(), y(), x());
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

}
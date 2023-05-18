#include "lchf.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    auto LChf::distance(const LChf &color0, const LChf &color1) -> float
    {
        constexpr float OneOver360 = 1.0 / 360.0;
        if (color0 == color1)
        {
            return 0.0F;
        }
        float dL = color0.L() - color1.L(); // [-1,1]
        float dC = color0.C() - color1.C(); // [-1,1]
        // use closest hue distance to make hue wrap around
        float dH0 = 2.0F * std::abs((color0.H() * OneOver360) - (color1.H() * OneOver360));
        float dH1 = 2.0F - dH0;
        float dH = dH0 < dH1 ? dH0 : dH1; // [0, 1]
        return (dL * dL + dC * dC + dH * dH) / 3.0F;
    } // max:  (   1    +    1    +    1    ) / 3 = 1

}
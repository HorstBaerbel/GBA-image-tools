#include "lchf.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    auto LChf::mse(const LChf &color0, const LChf &color1) -> float
    {
        constexpr float OneOver360 = 1.0 / 360.0;
        if (color0 == color1)
        {
            return 0.0F;
        }
        float dL = (color0.L() - color1.L()) / 100.0F; // [-1,1]
        float dC = (color0.C() - color1.C()) / 200.0F; // [-1,1]
        // use closest hue distance to make hue wrap around
        float dH0 = std::abs((color0.h() * OneOver360) - (color1.h() * OneOver360)); // [0,1]
        float dH1 = 1.0F - dH0;
        float dH = dH0 < dH1 ? dH0 : dH1; // [0,1]
        return (0.5F * dL * dL + 0.3F * dC * dC + 0.2F * dH * dH);
    } // max:  (0.5  *  1 *  1 + 0.3  *  1 *  1 + 0.2  *  1 *  1) = 1
}
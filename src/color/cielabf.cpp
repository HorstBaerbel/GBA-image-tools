#include "cielabf.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    auto CIELabf::mse(const CIELabf &color0, const CIELabf &color1) -> float
    {
        if (color0 == color1)
        {
            return 0.0F;
        }
        float dL = (color0.L() - color1.L()) / 100.0F;          // [0,1]
        float da = (color0.a() - color1.a() + 255.0F) / 510.0F; // [0,1]
        float db = (color0.b() - color1.b() + 255.0F) / 510.0F; // [0,1]
        return (dL * dL + da * da + db * db) / 3.0F;
    } // max:  ( 1 *  1 +  1 *  1 +  1 *  1) / 3 = 1
}
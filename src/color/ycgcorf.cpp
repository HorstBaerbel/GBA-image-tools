#include "ycgcorf.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    auto YCgCoRf::normalized() const -> YCgCoRf
    {
        return {Y(), 0.5F * (Cg() + 1.0F), 0.5F * (Co() + 1.0F)};
    }

    auto YCgCoRf::mse(const YCgCoRf &color0, const YCgCoRf &color1) -> float
    {
        if (color0 == color1)
        {
            return 0.0F;
        }
        auto dY = color0.Y() - color1.Y();                                    // [-1,1]
        auto dCg = 0.5F * (color0.Cg() + 1.0F) - 0.5F * (color1.Cg() + 1.0F); // [-1,1]
        auto dCo = 0.5F * (color0.Co() + 1.0F) - 0.5F * (color1.Co() + 1.0F); // [-1,1]
        return (0.5F * dY * dY + 0.25F * dCg * dCg + 0.25F * dCo * dCo);
    } // max:  (0.5  *  1 *  1 + 0.25  *   1 *   1 + 0.25  *   1 *   1) = 1
}
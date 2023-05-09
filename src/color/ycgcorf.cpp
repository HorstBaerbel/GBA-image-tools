#include "ycgcorf.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    auto YCgCoRf::normalized() const -> YCgCoRf
    {
        return {Y(), 0.5F * (Cg() + 1.0F), 0.5F * (Co() + 1.0F)};
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
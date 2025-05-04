#include "rgbf.h"

#include <Eigen/Core>
#include <Eigen/Dense>

namespace Color
{

    auto RGBf::swapToBGR() const -> RGBf
    {
        return RGBf(z(), y(), x());
    }
}
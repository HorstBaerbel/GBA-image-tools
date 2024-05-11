#include "grayf.h"

namespace Color
{

    auto Grayf::mse(const Grayf &color0, const Grayf &color1) -> float
    {
        auto dI = color1.I() - color0.I();
        return dI * dI;
    }

}

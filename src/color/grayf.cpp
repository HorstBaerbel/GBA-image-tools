#include "grayf.h"

namespace Color
{

    auto Grayf::distance(const Grayf &color0, const Grayf &color1) -> float
    {
        return std::abs(color1.I() - color0.I());
    }

}

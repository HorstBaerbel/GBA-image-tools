#pragma once

#include <cstdint>
#include <Eigen/Core>

namespace Color
{

    struct YCgCoR
    {
        float Y = 0;
        float Cg = 0;
        float Co = 0;

        static YCgCoR fromRGB888(const uint8_t *rgb888);
    }; // See: https://en.wikipedia.org/wiki/YCoCg
}

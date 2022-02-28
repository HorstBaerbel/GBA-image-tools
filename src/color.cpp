#include "color.h"

namespace Color
{

    YCgCoR YCgCoR::fromRGB888(const uint8_t *rgb888)
    {
        YCgCoR result;
        result.Co = static_cast<float>(rgb888[0]) - static_cast<float>(rgb888[2]);
        float tmp = static_cast<float>(rgb888[2]) + result.Co / 2.0;
        result.Cg = static_cast<float>(rgb888[1]) - tmp;
        result.Y = tmp + static_cast<float>(rgb888[1]) / 2.0;
        return result;
    }

}
#include "gamma.h"

namespace Color
{
    auto srgbToLinear(const RGBf &color) -> RGBf
    {
        auto c0 = color[0] <= 0.04045F ? (color[0] / 12.92F) : (std::powf((color[0] + 0.055F) / 1.055F, 2.4F));
        auto c1 = color[1] <= 0.04045F ? (color[1] / 12.92F) : (std::powf((color[1] + 0.055F) / 1.055F, 2.4F));
        auto c2 = color[2] <= 0.04045F ? (color[2] / 12.92F) : (std::powf((color[2] + 0.055F) / 1.055F, 2.4F));
        return {c0, c1, c2};
    }

    auto linearToSrgb(const RGBf &color) -> RGBf
    {
        auto c0 = color[0] <= 0.0031308F ? (color[0] * 12.92F) : (std::powf(color[0], 1.0F / 2.4F) * 1.055F - 0.055F);
        auto c1 = color[1] <= 0.0031308F ? (color[1] * 12.92F) : (std::powf(color[1], 1.0F / 2.4F) * 1.055F - 0.055F);
        auto c2 = color[2] <= 0.0031308F ? (color[2] * 12.92F) : (std::powf(color[2], 1.0F / 2.4F) * 1.055F - 0.055F);
        return {c0, c1, c2};
    }
}
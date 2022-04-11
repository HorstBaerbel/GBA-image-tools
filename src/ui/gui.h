#pragma once

#include <cstdint>
#include <vector>

namespace Ui
{

    class Window
    {
        virtual auto displayImageRGB888(const std::vector<uint8_t> &image, uint32_t width, uint32_t height, int32_t x = 0, int32_t y = 0) -> void = 0;
        virtual auto displayImageRGB555(const std::vector<uint8_t> &image, uint32_t width, uint32_t height, int32_t x = 0, int32_t y = 0) -> void = 0;
    };

}
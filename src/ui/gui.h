#pragma once

#include <cstdint>
#include <vector>

namespace Ui
{

    enum class ColorFormat
    {
        Unknown = 0,
        XRGB1555 = 15,
        RGB565 = 16,
        XBGR1555 = 17,
        BGR565 = 18,
        XRGB8888 = 32
    };

    class Window
    {
        virtual auto displayImage(const std::vector<uint8_t> &image, ColorFormat format, uint32_t width, uint32_t height, int32_t x = 0, int32_t y = 0) -> void = 0;
        virtual auto displayImage(const uint8_t *image, std::size_t size, ColorFormat format, uint32_t width, uint32_t height, int32_t x = 0, int32_t y = 0) -> void = 0;
    };

}
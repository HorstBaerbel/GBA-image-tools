#include "imagestructs.h"

namespace Image
{

    auto bitsPerPixel(const Frame &image) -> uint32_t
    {
        return Color::formatInfo(image.data.pixels().format()).bitsPerPixel;
    }

    auto bytesPerPixel(const Frame &image) -> uint32_t
    {
        return Color::formatInfo(image.data.pixels().format()).bytesPerPixel;
    }

    auto bitsPerColorMapEntry(const Frame &image) -> uint32_t
    {
        return Color::formatInfo(image.data.colorMap().format()).bitsPerPixel;
    }

    auto bytesPerColorMapEntry(const Frame &image) -> uint32_t
    {
        return Color::formatInfo(image.data.colorMap().format()).bytesPerPixel;
    }
}

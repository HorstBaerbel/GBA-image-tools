#include "imagestructs.h"

namespace Image
{

    auto bitsPerPixel(const Data &image) -> uint32_t
    {
        return Color::formatInfo(image.image.data.pixels().format()).bitsPerPixel;
    }

    auto bytesPerPixel(const Data &image) -> uint32_t
    {
        return Color::formatInfo(image.image.data.pixels().format()).bytesPerPixel;
    }

    auto bitsPerColorMapEntry(const Data &image) -> uint32_t
    {
        return Color::formatInfo(image.image.data.colorMap().format()).bitsPerPixel;
    }

    auto bytesPerColorMapEntry(const Data &image) -> uint32_t
    {
        return Color::formatInfo(image.image.data.colorMap().format()).bytesPerPixel;
    }
}

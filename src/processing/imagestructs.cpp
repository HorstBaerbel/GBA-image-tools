#include "imagestructs.h"

namespace Image
{

    auto bitsPerPixel(const Data &image) -> uint32_t
    {
        return Color::formatInfo(image.imageData.pixels().format()).bitsPerPixel;
    }

    auto bytesPerPixel(const Data &image) -> uint32_t
    {
        return Color::formatInfo(image.imageData.pixels().format()).bytesPerPixel;
    }

    auto bitsPerColorMapEntry(const Data &image) -> uint32_t
    {
        return Color::formatInfo(image.imageData.colorMap().format()).bitsPerPixel;
    }

    auto bytesPerColorMapEntry(const Data &image) -> uint32_t
    {
        return Color::formatInfo(image.imageData.colorMap().format()).bytesPerPixel;
    }

}
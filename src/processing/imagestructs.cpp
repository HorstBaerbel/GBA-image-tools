#include "imagestructs.h"

namespace Image
{

    auto isPaletted(const Data &frame) -> bool
    {
        return isPaletted(frame.colorFormat);
    }

    auto hasColorMap(const Data &frame) -> bool
    {
        switch (frame.colorMapFormat)
        {
        case Color::Format::Unknown:
            return false;
        case Color::Format::RGB555:
        case Color::Format::RGB565:
        case Color::Format::RGB888:
        case Color::Format::RGBf:
        case Color::Format::YCgCof:
            return frame.colorMap.size() > 0;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
        }
    }

    auto bytesPerColorMapEntry(const Data &frame) -> uint32_t
    {
        switch (frame.colorMapFormat)
        {
        case Color::Format::Unknown:
            return 0;
        case Color::Format::RGB555:
        case Color::Format::RGB565:
            return 2;
        case Color::Format::RGB888:
            return 3;
        case Color::Format::RGBf:
        case Color::Format::YCgCof:
            return 12;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
        }
    }

}
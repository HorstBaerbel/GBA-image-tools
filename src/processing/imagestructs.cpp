#include "imagestructs.h"

namespace Image
{

    auto isPaletted(const Data &frame) -> bool
    {
        switch (frame.colorFormat)
        {
        case Color::Format::Unknown:
        case Color::Format::RGB555:
        case Color::Format::RGB565:
        case Color::Format::RGB888:
        case Color::Format::RGBf:
        case Color::Format::YCgCod:
            return false;
            break;
        case Color::Format::Paletted1:
        case Color::Format::Paletted2:
        case Color::Format::Paletted4:
        case Color::Format::Paletted8:
            return true;
            break;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
            break;
        }
    }

    auto hasColorMap(const Data &frame) -> bool
    {
        switch (frame.colorMapFormat)
        {
        case Color::Format::Unknown:
            return false;
            break;
        case Color::Format::RGB555:
        case Color::Format::RGB565:
        case Color::Format::RGB888:
        case Color::Format::RGBf:
        case Color::Format::YCgCod:
            return frame.colorMap.size() > 0;
            break;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
            break;
        }
    }

    auto bytesPerColorMapEntry(const Data &frame) -> uint32_t
    {
        switch (frame.colorMapFormat)
        {
        case Color::Format::Unknown:
            return 0;
            break;
        case Color::Format::RGB555:
        case Color::Format::RGB565:
            return 2;
        case Color::Format::RGB888:
            return 3;
        case Color::Format::RGBf:
            return 12;
        case Color::Format::YCgCod:
            return 24;
            break;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
            break;
        }
    }

}
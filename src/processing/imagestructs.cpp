#include "imagestructs.h"

namespace Image
{

    bool hasColorMap(const Data &frame)
    {
        switch (frame.colorMapFormat)
        {
        case Color::Format::Unknown:
            return false;
            break;
        case Color::Format::RGB555:
        case Color::Format::RGB565:
        case Color::Format::RGB888:
        case Color::Format::RGBd:
        case Color::Format::YCgCod:
            return frame.colorMap.size() > 0;
            break;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
            break;
        }
    }

    uint32_t bytesPerColorMapEntry(const Data &frame)
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
        case Color::Format::RGBd:
        case Color::Format::YCgCod:
            return 24;
            break;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
            break;
        }
    }

}
#include "colorformat.h"

#include <stdexcept>

namespace Color
{

    uint32_t bitsPerPixelForFormat(Format format)
    {
        switch (format)
        {
        case Format::Paletted1:
            return 1;
        case Format::Paletted2:
            return 2;
        case Format::Paletted4:
            return 4;
        case Format::Paletted8:
            return 8;
        case Format::RGB555:
            return 15;
        case Format::RGB565:
            return 16;
        case Format::RGB888:
            return 24;
        case Format::XRGB888:
            return 32;
        case Format::RGBd:
        case Format::YCgCod:
            return 192;
        default:
            throw std::runtime_error("Bad color format");
        }
    }

    std::string to_string(Format format)
    {
        switch (format)
        {
        case Format::Paletted1:
            return "paletted 1-bit";
        case Format::Paletted2:
            return "paletted 2-bit";
        case Format::Paletted4:
            return "paletted 4-bit";
        case Format::Paletted8:
            return "paletted 8-bit";
        case Format::RGB555:
            return "RGB555";
        case Format::RGB565:
            return "RGB565";
        case Format::RGB888:
            return "RGB888";
        case Format::XRGB888:
            return "XRGB888";
        case Format::RGBd:
            return "RGB double";
        case Format::YCgCod:
            return "YCgCo double";
        default:
            throw std::runtime_error("Bad color format");
        }
    }

}

#include "colorformat.h"

#include "exception.h"

namespace Color
{

    auto isPaletted(Format format) -> bool
    {
        switch (format)
        {
        case Color::Format::Unknown:
        case Color::Format::RGB555:
        case Color::Format::RGB565:
        case Color::Format::RGB888:
        case Color::Format::RGBf:
        case Color::Format::YCgCof:
            return false;
        case Color::Format::Paletted1:
        case Color::Format::Paletted2:
        case Color::Format::Paletted4:
        case Color::Format::Paletted8:
            return true;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
        }
    }

    auto bitsPerPixelForFormat(Format format) -> uint32_t
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
        case Format::RGBf:
            return 96;
        case Format::YCgCof:
            return 96;
        default:
            THROW(std::runtime_error, "Bad color format");
        }
    }

    auto to_string(Format format) -> std::string
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
        case Format::RGBf:
            return "RGB float";
        case Format::YCgCof:
            return "YCgCo float";
        default:
            THROW(std::runtime_error, "Bad color format");
        }
    }

}

#include "colorformat.h"

#include "lchf.h"
#include "rgbf.h"
#include "xrgb1555.h"
#include "rgb565.h"
#include "xrgb8888.h"
#include "ycgcorf.h"
#include "exception.h"

namespace Color
{

    auto isIndexed(Format format) -> bool
    {
        switch (format)
        {
        case Color::Format::Unknown:
        case Color::Format::XRGB1555:
        case Color::Format::RGB565:
        case Color::Format::Lchf:
        case Color::Format::RGBf:
        case Color::Format::YCgCoRf:
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

    auto isTruecolor(Format format) -> bool
    {
        switch (format)
        {
        case Color::Format::XRGB1555:
        case Color::Format::RGB565:
        case Color::Format::Lchf:
        case Color::Format::RGBf:
        case Color::Format::YCgCoRf:
            return true;
        case Color::Format::Paletted1:
        case Color::Format::Paletted2:
        case Color::Format::Paletted4:
        case Color::Format::Paletted8:
            return false;
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
        case Format::XRGB1555:
            return 15;
        case Format::RGB565:
            return 16;
        case Format::XRGB8888:
            return 32;
        case Format::Lchf:
        case Format::RGBf:
        case Format::YCgCoRf:
            return 96;
        default:
            THROW(std::runtime_error, "Bad color format");
        }
    }

    auto bytesPerColorMapEntry(Format format) -> uint32_t
    {
        switch (format)
        {
        case Color::Format::Unknown:
            return 0;
        case Color::Format::XRGB1555:
        case Color::Format::RGB565:
            return 2;
        case Color::Format::XRGB8888:
            return 4;
        case Color::Format::Lchf:
        case Color::Format::RGBf:
        case Color::Format::YCgCoRf:
            return 12;
        default:
            THROW(std::runtime_error, "Unsupported color map format");
        }
    }

    auto toString(Format format) -> std::string
    {
        switch (format)
        {
        case Format::Paletted1:
            return "Paletted 1-bit";
        case Format::Paletted2:
            return "Paletted 2-bit";
        case Format::Paletted4:
            return "Paletted 4-bit";
        case Format::Paletted8:
            return "Paletted 8-bit";
        case Format::XRGB1555:
            return "XRGB1555";
        case Format::RGB565:
            return "RGB565";
        case Format::XRGB8888:
            return "XRGB8888";
        case Format::Lchf:
            return "Lch float";
        case Format::RGBf:
            return "RGB float";
        case Format::YCgCoRf:
            return "YCgCoR float";
        default:
            THROW(std::runtime_error, "Bad color format");
        }
    }

    template <>
    auto toFormat<uint8_t>() -> Format
    {
        return Format::Unknown;
    }

    template <>
    auto toFormat<XRGB1555>() -> Format
    {
        return Format::XRGB1555;
    }

    template <>
    auto toFormat<RGB565>() -> Format
    {
        return Format::RGB565;
    }

    template <>
    auto toFormat<XRGB8888>() -> Format
    {
        return Format::XRGB8888;
    }

    template <>
    auto toFormat<RGBf>() -> Format
    {
        return Format::RGBf;
    }

    template <>
    auto toFormat<Lchf>() -> Format
    {
        return Format::Lchf;
    }

    template <>
    auto toFormat<YCgCoRf>() -> Format
    {
        return Format::YCgCoRf;
    }

}

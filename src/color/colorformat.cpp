#include "colorformat.h"

#include "exception.h"
#include "grayf.h"
#include "lchf.h"
#include "rgb565.h"
#include "rgb888.h"
#include "rgbf.h"
#include "xrgb1555.h"
#include "xrgb8888.h"
#include "ycgcorf.h"

#include <map>

namespace Color
{

    static const std::map<Format, FormatInfo> FormatInfoMap = {
        {Format::Unknown, {Format::Unknown, "Unknown", 0, 0, 0, false, false, false}},
        {Format::Paletted1, {Format::Paletted1, "Paletted 1-bit", 1, 1, 1, true, false, false}},
        {Format::Paletted2, {Format::Paletted2, "Paletted 2-bit", 2, 1, 1, true, false, false}},
        {Format::Paletted4, {Format::Paletted4, "Paletted 4-bit", 4, 1, 1, true, false, false}},
        {Format::Paletted8, {Format::Paletted8, "Paletted 8-bit", 8, 1, 1, true, false, false}},
        {Format::XRGB1555, {Format::XRGB1555, "XRGB1555", 15, sizeof(Color::RGB565::pixel_type), Color::RGB565::Channels, false, true, false}},
        {Format::RGB565, {Format::RGB565, "RGB565", sizeof(Color::RGB565::pixel_type) * 8, sizeof(Color::RGB565::pixel_type), Color::RGB565::Channels, false, true, false}},
        {Format::RGB888, {Format::RGB888, "RGB888", sizeof(Color::RGB888::pixel_type) * 8, sizeof(Color::RGB888::pixel_type), Color::RGB888::Channels, false, true, false}},
        {Format::XRGB8888, {Format::XRGB8888, "XRGB8888", sizeof(Color::XRGB8888::pixel_type) * 8, sizeof(Color::XRGB8888::pixel_type), Color::XRGB8888::Channels, false, true, false}},
        {Format::LChf, {Format::LChf, "LCh float", sizeof(Color::LChf::pixel_type) * 8, sizeof(Color::LChf::pixel_type), Color::LChf::Channels, false, true, false}},
        {Format::RGBf, {Format::RGBf, "RGB float", sizeof(Color::RGBf::pixel_type) * 8, sizeof(Color::RGBf::pixel_type), Color::RGBf::Channels, false, true, false}},
        {Format::YCgCoRf, {Format::YCgCoRf, "YCgCoR float", sizeof(Color::YCgCoRf::pixel_type) * 8, sizeof(Color::YCgCoRf::pixel_type), Color::YCgCoRf::Channels, false, true, false}},
        {Format::Grayf, {Format::Grayf, "Grayscale float", sizeof(Color::Grayf::pixel_type) * 8, sizeof(Color::Grayf::pixel_type), Color::Grayf::Channels, false, false, false}}};

    auto formatInfo(Format format) -> const FormatInfo &
    {
        return FormatInfoMap.at(format);
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
    auto toFormat<RGB888>() -> Format
    {
        return Format::RGB888;
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
    auto toFormat<LChf>() -> Format
    {
        return Format::LChf;
    }

    template <>
    auto toFormat<YCgCoRf>() -> Format
    {
        return Format::YCgCoRf;
    }

    template <>
    auto toFormat<Grayf>() -> Format
    {
        return Format::Grayf;
    }

}

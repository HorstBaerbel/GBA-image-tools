#include "colorformat.h"

#include "grayf.h"
#include "lchf.h"
#include "rgbf.h"
#include "xrgb1555.h"
#include "rgb565.h"
#include "xrgb8888.h"
#include "ycgcorf.h"
#include "exception.h"

#include <map>

namespace Color
{

    static const std::map<Format, FormatInfo> FormatInfoMap = {
        {Format::Unknown, {Format::Unknown, "Unknown", 0, 0, 0, false, false}},
        {Format::Paletted1, {Format::Paletted1, "Paletted 1-bit", 1, 1, 1, true, false}},
        {Format::Paletted2, {Format::Paletted2, "Paletted 2-bit", 2, 1, 1, true, false}},
        {Format::Paletted4, {Format::Paletted4, "Paletted 4-bit", 4, 1, 1, true, false}},
        {Format::Paletted8, {Format::Paletted8, "Paletted 8-bit", 8, 1, 1, true, false}},
        {Format::XRGB1555, {Format::XRGB1555, "XRGB1555", 15, 2, 3, false, true}},
        {Format::RGB565, {Format::RGB565, "RGB565", 16, 2, 3, false, true}},
        {Format::XRGB8888, {Format::XRGB8888, "XRGB8888", 4 * 8, 4, 3, false, true}},
        {Format::LChf, {Format::LChf, "LCh float", 12 * 8, 12, 3, false, true}},
        {Format::RGBf, {Format::RGBf, "RGB float", 12 * 8, 12, 3, false, true}},
        {Format::YCgCoRf, {Format::YCgCoRf, "YCgCoR float", 12 * 8, 12, 3, false, true}},
        {Format::Grayf, {Format::Grayf, "Grayscale float", 4 * 8, 4, 1, false, false}}};

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

#include "colorhelpers.h"

#include "conversions.h"
#include "grayf.h"
#include "lchf.h"
#include "rgb565.h"
#include "rgb888.h"
#include "rgbf.h"
#include "xrgb1555.h"
#include "xrgb8888.h"
#include "ycgcorf.h"

#include <algorithm>

namespace ColorHelpers
{

    auto addColorAtIndex0(const std::vector<Color::XRGB8888> &colorMap, const Color::XRGB8888 &color0) -> std::vector<Color::XRGB8888>
    {
        std::vector<Color::XRGB8888> tempMap(colorMap.size() + 1, color0);
        std::copy(colorMap.cbegin(), colorMap.cend(), std::next(tempMap.begin()));
        return tempMap;
    }

    auto swapColors(const std::vector<Color::XRGB8888> &colors, const std::vector<uint8_t> &newIndices) -> std::vector<Color::XRGB8888>
    {
        std::vector<Color::XRGB8888> result;
        for (uint32_t i = 0; i < colors.size(); i++)
        {
            result.push_back(colors.at(newIndices[i]));
        }
        return result;
    }

    auto buildColorMapRGB555() -> std::vector<Color::XRGB8888>
    {
        std::vector<Color::XRGB8888> result;
        for (uint32_t r = 0; r < 32; ++r)
        {
            const auto cr = static_cast<uint8_t>((255 * r) / 31);
            for (uint32_t g = 0; g < 32; ++g)
            {
                const auto cg = static_cast<uint8_t>((255 * g) / 31);
                for (uint32_t b = 0; b < 32; ++b)
                {
                    const auto cb = static_cast<uint8_t>((255 * b) / 31);
                    result.emplace_back(Color::XRGB8888(cr, cg, cb));
                }
            }
        }
        return result;
    }

    auto buildColorMapRGB565() -> std::vector<Color::XRGB8888>
    {
        std::vector<Color::XRGB8888> result;
        for (uint32_t r = 0; r < 32; ++r)
        {
            const auto cr = static_cast<uint8_t>((255 * r) / 31);
            for (uint32_t g = 0; g < 64; ++g)
            {
                const auto cg = static_cast<uint8_t>((255 * g) / 63);
                for (uint32_t b = 0; b < 32; ++b)
                {
                    const auto cb = static_cast<uint8_t>((255 * b) / 31);
                    result.emplace_back(Color::XRGB8888(cr, cg, cb));
                }
            }
        }
        return result;
    }

    auto buildColorMapFor(Color::Format format) -> std::vector<Color::XRGB8888>
    {
        if (format == Color::Format::XRGB1555)
        {
            return buildColorMapRGB555();
        }
        else if (format == Color::Format::RGB565)
        {
            return buildColorMapRGB565();
        }
        THROW(std::runtime_error, "Unsupported color format");
    }

    auto toXRGB8888(const std::vector<uint8_t> &pixels, Color::Format pixelFormat) -> std::vector<Color::XRGB8888>
    {
        REQUIRE(!pixels.empty(), std::runtime_error, "Pixels can not be empty");
        REQUIRE(pixelFormat != Color::Format::Unknown, std::runtime_error, "Bad pixel format");
        auto pixelFormatInfo = Color::formatInfo(pixelFormat);
        REQUIRE(pixelFormatInfo.isTruecolor, std::runtime_error, "Pixels must be in true-color format");
        // convert raw data to XRGB8888 or swapped
        std::vector<Color::XRGB8888> result;
        switch (pixelFormat)
        {
        case Color::Format::XRGB1555:
        case Color::Format::XBGR1555:
            result = Color::convertTo<Color::XRGB8888, Color::XRGB1555>(pixels);
            break;
        case Color::Format::RGB565:
        case Color::Format::BGR565:
            result = Color::convertTo<Color::XRGB8888, Color::RGB565>(pixels);
            break;
        case Color::Format::RGB888:
        case Color::Format::BGR888:
            result = Color::convertTo<Color::XRGB8888, Color::RGB888>(pixels);
            break;
        case Color::Format::XRGB8888:
        case Color::Format::XBGR8888:
            result = Color::convertTo<Color::XRGB8888, Color::XRGB8888>(pixels);
            break;
        case Color::Format::RGBf:
            result = Color::convertTo<Color::XRGB8888, Color::RGBf>(pixels);
            break;
        case Color::Format::LChf:
            result = Color::convertTo<Color::XRGB8888, Color::LChf>(pixels);
            break;
        case Color::Format::YCgCoRf:
            result = Color::convertTo<Color::XRGB8888, Color::YCgCoRf>(pixels);
            break;
        case Color::Format::Grayf:
            result = Color::convertTo<Color::XRGB8888, Color::Grayf>(pixels);
            break;
        default:
            THROW(std::runtime_error, "Unsupported pixel format");
        }
        // swap red<->blue if we need to
        if (pixelFormatInfo.hasSwappedRedBlue)
        {
            std::for_each(result.begin(), result.end(), [](auto &c)
                          { return c.swapToBGR(); });
        }
        return result;
    }

    auto toXRGB8888(const std::vector<uint8_t> &pixels, Color::Format pixelFormat, const std::vector<Color::XRGB8888> &colorMap) -> std::vector<Color::XRGB8888>
    {
        REQUIRE(!pixels.empty(), std::runtime_error, "Pixels can not be empty");
        REQUIRE(pixelFormat != Color::Format::Unknown, std::runtime_error, "Bad pixel format");
        auto pixelFormatInfo = Color::formatInfo(pixelFormat);
        REQUIRE((pixelFormatInfo.isTruecolor && colorMap.empty()) || (pixelFormatInfo.isIndexed && !colorMap.empty()), std::runtime_error, "Pixels format color map mismatch");
        if (pixelFormatInfo.isIndexed)
        {
            // convert pixels
            std::vector<Color::XRGB8888> resultPixels;
            switch (pixelFormat)
            {
            case Color::Format::Paletted8:
                std::transform(pixels.cbegin(), pixels.cend(), std::back_inserter(resultPixels), [&colorMap](auto i)
                               { return colorMap.at(i); });
                break;
            default:
                THROW(std::runtime_error, "Unsupported pixel format");
                break;
            }
            return resultPixels;
        }
        else
        {
            // convert true-color
            return toXRGB8888(pixels, pixelFormat);
        }
    }
}

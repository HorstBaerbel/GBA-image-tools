#pragma once

#include "color/colorformat.h"
#include "color/conversions.h"
#include "color/grayf.h"
#include "color/lchf.h"
#include "color/rgb565.h"
#include "color/rgbf.h"
#include "color/xrgb1555.h"
#include "color/xrgb8888.h"
#include "color/ycgcorf.h"
#include "datahelpers.h"
#include "datasize.h"
#include "exception.h"
#include "pixeldata.h"

#include <cstdint>
#include <vector>

namespace Image
{

    /// @brief Stores indexed images with a color map, true color images or raw / compressed image data
    /// Continuous array of pixels, no stride etc.
    class ImageData
    {
    public:
        ImageData() = default;

        template <typename COLORMAP_FORMAT>
        ImageData(const std::vector<uint8_t> &indices, Color::Format pixelFormat, const std::vector<COLORMAP_FORMAT> &colorMap)
            : m_pixels(indices, pixelFormat), m_colorMap(colorMap, Color::toFormat<COLORMAP_FORMAT>())
        {
            auto colorMapFormat = Color::toFormat<COLORMAP_FORMAT>();
            REQUIRE(pixelFormat == Color::Format::Paletted1 || pixelFormat == Color::Format::Paletted2 || pixelFormat == Color::Format::Paletted4 || pixelFormat == Color::Format::Paletted8, std::runtime_error, "Pixel format must be paletted");
            REQUIRE(colorMapFormat == Color::Format::XRGB1555 || colorMapFormat == Color::Format::RGB565 ||
                        colorMapFormat == Color::Format::XRGB8888 || colorMapFormat == Color::Format::RGBf ||
                        colorMapFormat == Color::Format::LChf || colorMapFormat == Color::Format::YCgCoRf,
                    std::runtime_error, "Color map format must be XRGB1555, RGB565, XRGB8888, RGBf, LChf or YCgCoRf");
        }

        template <typename COLORMAP_FORMAT>
        ImageData(std::vector<uint8_t> &&indices, Color::Format pixelFormat, std::vector<COLORMAP_FORMAT> &&colorMap)
        {
            auto colorMapFormat = Color::toFormat<COLORMAP_FORMAT>();
            REQUIRE(pixelFormat == Color::Format::Paletted1 || pixelFormat == Color::Format::Paletted2 || pixelFormat == Color::Format::Paletted4 || pixelFormat == Color::Format::Paletted8, std::runtime_error, "Pixel format must be paletted");
            REQUIRE(colorMapFormat == Color::Format::XRGB1555 || colorMapFormat == Color::Format::RGB565 ||
                        colorMapFormat == Color::Format::XRGB8888 || colorMapFormat == Color::Format::RGBf ||
                        colorMapFormat == Color::Format::LChf || colorMapFormat == Color::Format::YCgCoRf,
                    std::runtime_error, "Color map format must be XRGB1555, RGB565, XRGB8888, RGBf, LChf or YCgCoRf");
            m_pixels = PixelData(std::move(indices), pixelFormat);
            m_colorMap = PixelData(std::move(colorMap), colorMapFormat);
        }

        template <typename PIXEL_FORMAT>
        ImageData(const std::vector<PIXEL_FORMAT> &pixels)
        {
            auto pixelFormat = Color::toFormat<PIXEL_FORMAT>();
            REQUIRE(pixelFormat == Color::Format::XRGB1555 ||
                        pixelFormat == Color::Format::RGB565 ||
                        pixelFormat == Color::Format::XRGB8888 ||
                        pixelFormat == Color::Format::RGBf ||
                        pixelFormat == Color::Format::LChf ||
                        pixelFormat == Color::Format::YCgCoRf ||
                        pixelFormat == Color::Format::Grayf,
                    std::runtime_error, "Pixel format must be XRGB1555, RGB565, XRGB8888, RGBf, LChf or YCgCoRf, Grayf");
            m_pixels = PixelData(pixels, pixelFormat);
        }

        template <typename PIXEL_FORMAT>
        ImageData(std::vector<PIXEL_FORMAT> &&pixels)
        {
            auto pixelFormat = Color::toFormat<PIXEL_FORMAT>();
            REQUIRE(pixelFormat == Color::Format::XRGB1555 ||
                        pixelFormat == Color::Format::RGB565 ||
                        pixelFormat == Color::Format::XRGB8888 ||
                        pixelFormat == Color::Format::RGBf ||
                        pixelFormat == Color::Format::LChf ||
                        pixelFormat == Color::Format::YCgCoRf ||
                        pixelFormat == Color::Format::Grayf,
                    std::runtime_error, "Pixel format must be XRGB1555, RGB565, XRGB8888, RGBf, LChf or YCgCoRf, Grayf");
            m_pixels = PixelData(std::move(pixels), pixelFormat);
        }

        ImageData(const std::vector<uint8_t> &rawData)
            : m_pixels(rawData, Color::Format::Unknown)
        {
        }

        ImageData(std::vector<uint8_t> &&rawData)
            : m_pixels(std::move(rawData), Color::Format::Unknown)
        {
        }

        auto pixels() const -> const PixelData &
        {
            return m_pixels;
        }

        auto pixels() -> PixelData &
        {
            return m_pixels;
        }

        auto colorMap() const -> const PixelData &
        {
            return m_colorMap;
        }

        auto colorMap() -> PixelData &
        {
            return m_colorMap;
        }

    private:
        PixelData m_pixels;
        PixelData m_colorMap;
    };
}

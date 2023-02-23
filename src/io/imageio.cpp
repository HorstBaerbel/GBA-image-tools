#include "imageio.h"

#include <Magick++.h>

#include <filesystem>

namespace IO
{

    auto writePaletted(Magick::Image &dst, const Image::Data &src) -> void
    {
        // write index data
        MagickCore::PixelPacket *pixels = dst.getPixels(0, 0, dst.columns(), dst.rows());
        MagickCore::IndexPacket *indices = dst.getIndexes();
        REQUIRE(indices != nullptr, std::runtime_error, "Bad indices pointer");
        const auto nrOfPixels = dst.columns() * dst.rows();
        for (std::size_t i = 0; i < nrOfPixels; i++)
        {
            *indices++ = src.data[i];
        }
        // write color map
        double maxR = 0.0;
        double maxG = 0.0;
        double maxB = 0.0;
        switch (src.colorMapFormat)
        {
        case Color::Format::RGB555:
            maxR = 31.0;
            maxG = 31.0;
            maxB = 31.0;
            break;
        case Color::Format::RGB565:
            maxR = 31.0;
            maxG = 63.0;
            maxB = 31.0;
            break;
        case Color::Format::RGB888:
            maxR = 255.0;
            maxG = 255.0;
            maxB = 255.0;
            break;
        default:
            THROW(std::runtime_error, "Unsupported image format");
        }
        const auto nrOfColors = src.colorMapData.size() / 3;
        for (std::size_t i = 0; i < nrOfColors; i++)
        {
            const auto index = i * 3;
            Magick::Color color;
            color.redQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.colorMapData[index]) / maxR));
            color.greenQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.colorMapData[index + 1]) / maxG));
            color.blueQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.colorMapData[index + 2]) / maxB));
            dst.colorMap(i, color);
        }
    }

    auto writeTruecolor(Magick::Image &dst, const Image::Data &src) -> void
    {
        double maxR = 0.0;
        double maxG = 0.0;
        double maxB = 0.0;
        switch (src.colorFormat)
        {
        case Color::Format::RGB555:
            maxR = 31.0;
            maxG = 31.0;
            maxB = 31.0;
            break;
        case Color::Format::RGB565:
            maxR = 31.0;
            maxG = 63.0;
            maxB = 31.0;
            break;
        case Color::Format::RGB888:
            maxR = 255.0;
            maxG = 255.0;
            maxB = 255.0;
            break;
        default:
            THROW(std::runtime_error, "Unsupported image format");
        }
        MagickCore::PixelPacket *pixels = dst.getPixels(0, 0, dst.columns(), dst.rows());
        const auto nrOfPixels = dst.columns() * dst.rows();
        for (std::size_t i = 0; i < nrOfPixels; i++)
        {
            const auto index = i * 3;
            Magick::Color color;
            color.redQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.data[index]) / maxR));
            color.greenQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.data[index + 1]) / maxG));
            color.blueQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.data[index + 2]) / maxB));
            *pixels++ = color;
        }
    }

    auto File::writeImage(const std::string &folder, const Image::Data &image) -> void
    {
        REQUIRE(image.colorFormat != Color::Format::Unknown, std::runtime_error, "Bad color format");
        REQUIRE(image.size.width() > 0 && image.size.height() > 0, std::runtime_error, "Bad image size");
        Magick::Image temp({image.size.width(), image.size.height()}, "black");
        const bool isPaletted = Image::isPaletted(image) && Image::hasColorMap(image);
        temp.type(isPaletted ? Magick::ImageType::PaletteType : Magick::ImageType::TrueColorType);
        temp.modifyImage();
        if (isPaletted)
        {
            writePaletted(temp, image);
        }
        else
        {
            writeTruecolor(temp, image);
        }
        temp.syncPixels();
        temp.write(std::filesystem::path(folder) / std::filesystem::path(image.fileName).filename());
    }

    auto File::writeImages(const std::string &folder, const std::vector<Image::Data> &images) -> void
    {
        for (const auto &i : images)
        {
            writeImage(folder, i);
        }
    }
}

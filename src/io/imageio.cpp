#include "imageio.h"

#include <Magick++.h>

namespace IO
{

    auto writePaletted(Magick::Image &dst, const Image::Data &src) -> void
    {
        // write index data
        MagickCore::IndexPacket *indices = dst.getIndexes();
        for (uint32_t i = 0; i < src.data.size(); i++)
        {
            *indices++ = src.data[i];
        }
        // write color map
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
        for (uint32_t i = 0; i < src.colorMapData.size(); i += 3)
        {
            Magick::Color color;
            color.redQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.colorMapData[i]) / maxR));
            color.greenQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.colorMapData[i + 1]) / maxG));
            color.blueQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.colorMapData[i + 2]) / maxB));
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
        for (uint32_t i = 0; i < src.data.size(); i += 3)
        {
            Magick::Color color;
            color.redQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.data[i]) / maxR));
            color.greenQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.data[i + 1]) / maxG));
            color.blueQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(src.data[i + 2]) / maxB));
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
        temp.write(folder + "result_" + image.fileName);
    }

    auto File::writeImages(const std::string &folder, const std::vector<Image::Data> &images) -> void
    {
        for (const auto &i : images)
        {
            writeImage(folder, i);
        }
    }
}

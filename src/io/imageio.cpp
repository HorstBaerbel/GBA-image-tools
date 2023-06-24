#include "imageio.h"

#include "exception.h"

#include <Magick++.h>

#include <filesystem>

namespace IO
{

    auto getColorMap(const Magick::Image &img) -> std::vector<Color::XRGB8888>
    {
        std::vector<Color::XRGB8888> colorMap(img.colorMapSize());
        for (std::remove_const<decltype(colorMap.size())>::type i = 0; i < colorMap.size(); ++i)
        {
            auto color = img.colorMap(i);
            auto r = static_cast<uint8_t>(255.0 * Magick::Color::scaleQuantumToDouble(color.redQuantum()));
            auto g = static_cast<uint8_t>(255.0 * Magick::Color::scaleQuantumToDouble(color.greenQuantum()));
            auto b = static_cast<uint8_t>(255.0 * Magick::Color::scaleQuantumToDouble(color.blueQuantum()));
            colorMap.emplace_back(Color::XRGB8888(r, g, b));
        }
        return colorMap;
    }

    auto getImageData(const Magick::Image &img) -> Image::ImageData
    {
        if (img.type() == Magick::ImageType::PaletteType)
        {
            // convert to linear RGB color space
            auto linearImg = img;
            linearImg.colorSpace(Magick::RGBColorspace);
            // get pixel indices
            const auto nrOfColors = linearImg.colorMapSize();
            REQUIRE(nrOfColors <= 256, std::runtime_error, "Only up to 256 colors supported in color map");
            const auto nrOfIndices = linearImg.columns() * linearImg.rows();
            auto srcPixels = linearImg.getConstPixels(0, 0, linearImg.columns(), linearImg.rows()); // we need to call this first for getIndices to work...
            auto srcIndices = linearImg.getConstIndexes();
            std::vector<uint8_t> dstIndices;
            for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; ++i)
            {
                dstIndices.push_back(srcIndices[i]);
            }
            return Image::ImageData(dstIndices, Color::Format::Paletted8, getColorMap(linearImg));
        }
        else if (img.type() == Magick::ImageType::TrueColorType)
        {
            std::vector<Color::XRGB8888> dstPixels;
            // convert to linear RGB color space
            auto linearImg = img;
            linearImg.colorSpace(Magick::RGBColorspace);
            // get pixel colors as RGBf
            const auto nrOfPixels = linearImg.columns() * linearImg.rows();
            auto srcPixels = linearImg.getConstPixels(0, 0, linearImg.columns(), linearImg.rows());
            for (std::remove_const<decltype(nrOfPixels)>::type i = 0; i < nrOfPixels; ++i)
            {
                auto pixel = srcPixels[i];
                auto r = static_cast<uint8_t>(255.0 * Magick::Color::scaleQuantumToDouble(pixel.red));
                auto g = static_cast<uint8_t>(255.0 * Magick::Color::scaleQuantumToDouble(pixel.green));
                auto b = static_cast<uint8_t>(255.0 * Magick::Color::scaleQuantumToDouble(pixel.blue));
                dstPixels.emplace_back(Color::XRGB8888(r, g, b));
            }
            return Image::ImageData(dstPixels);
        }
        THROW(std::runtime_error, "Unsupported image type");
    }

    auto readImage(const std::string &filePath) -> Image::Data
    {
        Magick::Image img;
        try
        {
            img.read(filePath);
        }
        catch (const Magick::Exception &ex)
        {
            THROW(std::runtime_error, "Failed to read image: " << ex.what());
        }
        Image::Data data;
        data.size = {img.size().width(), img.size().height()};
        auto imageData = getImageData(img);
        return data;
    }

    auto writePaletted(Magick::Image &dst, const Image::ImageData &src) -> void
    {
        // write index data
        MagickCore::PixelPacket *dstPixels = dst.getPixels(0, 0, dst.columns(), dst.rows());
        MagickCore::IndexPacket *dstIndices = dst.getIndexes();
        REQUIRE(dstIndices != nullptr, std::runtime_error, "Bad indices pointer");
        const auto nrOfPixels = dst.columns() * dst.rows();
        auto srcIndices = src.getPixels<uint8_t>();
        for (std::size_t i = 0; i < nrOfPixels; i++)
        {
            *dstIndices++ = srcIndices[i];
        }
        // write color map
        auto srcColors = src.convertColorMap<Color::XRGB8888>();
        const auto nrOfColors = srcColors.size() / 3;
        for (std::size_t i = 0; i < nrOfColors; i++)
        {
            Magick::Color color;
            color.redQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(srcColors[i].R()) / 255.0));
            color.greenQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(srcColors[i].G()) / 255.0));
            color.blueQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(srcColors[i].B()) / 255.0));
            dst.colorMap(i, color);
        }
    }

    auto writeTrueColor(Magick::Image &dst, const Image::ImageData &src) -> void
    {
        auto srcPixels = src.convertPixels<Color::XRGB8888>();
        MagickCore::PixelPacket *dstPixels = dst.getPixels(0, 0, dst.columns(), dst.rows());
        const auto nrOfPixels = dst.columns() * dst.rows();
        for (std::size_t i = 0; i < nrOfPixels; i++)
        {
            Magick::Color color;
            color.redQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(srcPixels[i].R()) / 255.0));
            color.greenQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(srcPixels[i].G()) / 255.0));
            color.blueQuantum(Magick::Color::scaleDoubleToQuantum(static_cast<double>(srcPixels[i].B()) / 255.0));
            *dstPixels++ = color;
        }
    }

    auto File::writeImage(const std::string &folder, const Image::Data &image) -> void
    {
        REQUIRE(image.imageData.pixelFormat() != Color::Format::Unknown, std::runtime_error, "Bad color format");
        REQUIRE(image.size.width() > 0 && image.size.height() > 0, std::runtime_error, "Bad image size");
        Magick::Image temp({image.size.width(), image.size.height()}, "black");
        const bool isIndexed = image.imageData.isIndexed();
        temp.type(isIndexed ? Magick::ImageType::PaletteType : Magick::ImageType::TrueColorType);
        temp.modifyImage();
        if (isIndexed)
        {
            writePaletted(temp, image.imageData);
        }
        else
        {
            writeTrueColor(temp, image.imageData);
        }
        temp.syncPixels();
        // convert to sRGB color space
        temp.colorSpace(Magick::sRGBColorspace);
        // write to disk
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

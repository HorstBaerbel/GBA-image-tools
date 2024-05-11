#include "imageio.h"

#include "exception.h"

#include <Magick++.h>

#include <filesystem>
#include <fstream>

using namespace MagickCore;

namespace IO
{

    auto getColorMap(const Magick::Image &img) -> std::vector<Color::XRGB8888>
    {
        REQUIRE(img.classType() == Magick::ClassType::PseudoClass, std::runtime_error, "Image must be paletted");
        if (img.type() == Magick::ImageType::GrayscaleType)
        {
            std::vector<Color::XRGB8888> colorMap;
            for (std::vector<Magick::Color>::size_type i = 0; i < 256; ++i)
            {
                colorMap.emplace_back(Color::XRGB8888(i, i, i));
            }
            return colorMap;
        }
        else if (img.type() == Magick::ImageType::PaletteType || img.type() == Magick::ImageType::PaletteAlphaType)
        {
            std::vector<Color::XRGB8888> colorMap;
            for (std::remove_const<decltype(colorMap.size())>::type i = 0; i < colorMap.size(); ++i)
            {
                auto color = Magick::ColorRGB(img.colorMap(i));
                auto r = static_cast<uint8_t>(255.0 * color.red());
                auto g = static_cast<uint8_t>(255.0 * color.green());
                auto b = static_cast<uint8_t>(255.0 * color.blue());
                colorMap.emplace_back(Color::XRGB8888(r, g, b));
            }
            return colorMap;
        }
        THROW(std::runtime_error, "Unsupported image type");
    }

    auto getImageData(const Magick::Image &img) -> Image::ImageData
    {
        if (img.classType() == Magick::ClassType::PseudoClass && img.type() == Magick::ImageType::GrayscaleType)
        {
            // get GrayChannel from image
            auto temp = img.separate(MagickCore::GrayChannel);
            auto indices = temp.getConstPixels(0, 0, temp.columns(), temp.rows());
            REQUIRE(indices != nullptr, std::runtime_error, "Failed to get grayscale image pixels");
            const auto nrOfIndices = temp.columns() * temp.rows();
            std::vector<uint8_t> dstIndices;
            for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i++)
            {
                dstIndices.push_back(static_cast<uint8_t>(std::round(255.0F * indices[i]) / QuantumRange));
            }
            return Image::ImageData(dstIndices, Color::Format::Paletted8, getColorMap(img));
        }
        else if (img.type() == Magick::ImageType::PaletteType || img.type() == Magick::ImageType::PaletteAlphaType)
        {
            // get pixel indices
            const auto nrOfColors = img.colorMapSize();
            REQUIRE(nrOfColors <= 256, std::runtime_error, "Only up to 256 colors supported in color map");
            // get IndexChannel from image
            auto temp = img.separate(MagickCore::IndexChannel);
            auto srcIndices = temp.getConstPixels(0, 0, temp.columns(), temp.rows());
            REQUIRE(srcIndices != nullptr, std::runtime_error, "Failed to get paletted image pixels");
            const auto nrOfIndices = temp.columns() * temp.rows();
            std::vector<uint8_t> dstIndices;
            for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i++)
            {
                REQUIRE(srcIndices[i] <= 255, std::runtime_error, "Image color index must be <= 255");
                dstIndices.push_back(static_cast<uint8_t>(srcIndices[i]));
            }
            return Image::ImageData(dstIndices, Color::Format::Paletted8, getColorMap(img));
        }
        else if (img.type() == Magick::ImageType::TrueColorType || img.type() == Magick::ImageType::TrueColorAlphaType)
        {
            std::vector<Color::XRGB8888> dstPixels;
            //  get pixel colors as RGBf
            auto srcPixels = img.getConstPixels(0, 0, img.columns(), img.rows());
            REQUIRE(srcPixels != nullptr, std::runtime_error, "Failed to get truecolor image pixels");
            const auto nrOfPixels = img.columns() * img.rows();
            for (std::remove_const<decltype(nrOfPixels)>::type i = 0; i < nrOfPixels; i++)
            {
                auto r = static_cast<uint8_t>((255.0 * *srcPixels++) / QuantumRange);
                auto g = static_cast<uint8_t>((255.0 * *srcPixels++) / QuantumRange);
                auto b = static_cast<uint8_t>((255.0 * *srcPixels++) / QuantumRange);
                // ignore alpha channel pixels
                if (img.type() == Magick::ImageType::TrueColorAlphaType)
                {
                    srcPixels++;
                }
                dstPixels.emplace_back(Color::XRGB8888(r, g, b));
            }
            return Image::ImageData(dstPixels);
        }
        THROW(std::runtime_error, "Unsupported image type");
    }

    auto File::readImage(const std::string &filePath) -> Image::Data
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
        data.imageData = getImageData(img);
        return data;
    }

    auto writePaletted(Magick::Image &dst, const Image::ImageData &src) -> void
    {
        // write index data
        auto dstPixels = dst.getPixels(0, 0, dst.columns(), dst.rows());
        auto dstIndices = static_cast<uint8_t *>(dst.getMetacontent());
        REQUIRE(dstIndices != nullptr, std::runtime_error, "Bad indices pointer");
        const auto nrOfPixels = dst.columns() * dst.rows();
        auto srcIndices = src.pixels().convertDataToRaw();
        for (std::size_t i = 0; i < nrOfPixels; i++)
        {
            *dstIndices++ = srcIndices[i];
        }
        // write color map
        auto srcColors = src.colorMap().convertData<Color::XRGB8888>();
        const auto nrOfColors = srcColors.size() / 3;
        for (std::size_t i = 0; i < nrOfColors; i++)
        {
            Magick::ColorRGB color;
            color.red(static_cast<double>(srcColors[i].R()) / 255.0);
            color.green(static_cast<double>(srcColors[i].G()) / 255.0);
            color.blue(static_cast<double>(srcColors[i].B()) / 255.0);
            dst.colorMap(i, color);
        }
    }

    auto writeTrueColor(Magick::Image &dst, const Image::ImageData &src) -> void
    {
        auto srcPixels = src.pixels().convertData<Color::XRGB8888>();
        auto dstPixels = dst.getPixels(0, 0, dst.columns(), dst.rows());
        const auto nrOfPixels = dst.columns() * dst.rows();
        for (std::size_t i = 0; i < nrOfPixels; i++)
        {
            *dstPixels++ = (QuantumRange * static_cast<double>(srcPixels[i].R())) / 255.0;
            *dstPixels++ = (QuantumRange * static_cast<double>(srcPixels[i].G())) / 255.0;
            *dstPixels++ = (QuantumRange * static_cast<double>(srcPixels[i].B())) / 255.0;
        }
    }

    auto File::writeImage(const Image::Data &image, const std::string &folder, const std::string &fileName) -> void
    {
        REQUIRE(image.imageData.pixels().format() != Color::Format::Unknown, std::runtime_error, "Bad color format");
        REQUIRE(image.size.width() > 0 && image.size.height() > 0, std::runtime_error, "Bad image size");
        REQUIRE(!image.fileName.empty() || !fileName.empty(), std::runtime_error, "Either image.fileName or fileName must contain a file name");
        Magick::Image temp({image.size.width(), image.size.height()}, "black");
        const bool isIndexed = !image.imageData.colorMap().empty();
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
        // create paths if neccessary
        auto outName = !fileName.empty() ? fileName : image.fileName;
        auto outPath = std::filesystem::path(folder) / std::filesystem::path(outName).filename();
        if (!std::filesystem::exists(std::filesystem::path(folder)))
        {
            std::filesystem::create_directory(std::filesystem::path(folder));
        }
        // write to disk
        temp.write(outPath);
    }

    auto File::writeImages(const std::vector<Image::Data> &images, const std::string &folder) -> void
    {
        for (const auto &i : images)
        {
            writeImage(i, folder);
        }
    }

    auto File::writeRawImage(const Image::Data &image, const std::string &folder, const std::string &fileName) -> void
    {
        REQUIRE(image.imageData.pixels().format() != Color::Format::Unknown, std::runtime_error, "Bad color format");
        REQUIRE(image.size.width() > 0 && image.size.height() > 0, std::runtime_error, "Bad image size");
        REQUIRE(!image.fileName.empty() || !fileName.empty(), std::runtime_error, "Either image.fileName or fileName must contain a file name");
        // create paths if neccessary
        auto outName = !fileName.empty() ? fileName : image.fileName;
        auto outPath = std::filesystem::path(folder) / std::filesystem::path(outName).filename();
        if (!std::filesystem::exists(std::filesystem::path(folder)))
        {
            std::filesystem::create_directory(std::filesystem::path(folder));
        }
        // write to disk
        auto ofs = std::ofstream(outPath, std::ios::binary);
        auto pixels = image.imageData.pixels().convertDataToRaw();
        ofs.write(reinterpret_cast<const char *>(pixels.data()), pixels.size());
    }
}

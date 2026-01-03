#include "imageio.h"

#include "exception.h"

#include "libplum.h"

#include <filesystem>
#include <fstream>

namespace IO
{

    auto getImageData(const plum_image *img) -> Image::ImageData
    {
        if (img->max_palette_index > 0 && img->data8 != nullptr && img->palette32 != nullptr)
        {
            // paletted image. read indices
            const auto nrOfPixels = img->width * img->height;
            std::vector<uint8_t> dstIndices(nrOfPixels);
            const uint8_t *srcPixels = img->data8;
            std::memcpy(dstIndices.data(), srcPixels, nrOfPixels);
            // read color map
            const uint32_t *srcColors = img->palette32;
            std::vector<Color::XRGB8888> dstColorMap;
            for (uint32_t i = 0; i <= img->max_palette_index; ++i)
            {
                auto pixel = *srcColors++;
                dstColorMap.emplace_back(Color::XRGB8888(PLUM_RED_32(pixel), PLUM_GREEN_32(pixel), PLUM_BLUE_32(pixel)));
            }
            return Image::ImageData(dstIndices, Color::Format::Paletted8, dstColorMap);
        }
        else if (img->data32 != nullptr)
        {
            // greyscale or true-color image
            // TODO: libplum currently has no real greyscale support, see: https://github.com/aaaaaa123456789/libplum/issues/20
            // so greyscale images are returned as PLUM_COLOR_32 RGBA8888 images
            // Check the metadata if the color depth of the greyscale channel is 8
            bool isGreyscale = false;
            const plum_metadata *metaData = img->metadata;
            while (metaData != nullptr)
            {
                // try to find the correct metadata node. also must have 5 entries to contain a greyscale channel
                if (metaData->type == PLUM_METADATA_COLOR_DEPTH && metaData->data != nullptr && metaData->size == 5)
                {
                    isGreyscale = reinterpret_cast<const uint8_t *>(metaData->data)[4] == 8;
                    break;
                }
                metaData = metaData->next;
            }
            // extract image data
            if (isGreyscale)
            {
                // read indices and keep track of maximum index value
                const auto nrOfPixels = img->width * img->height;
                std::vector<uint8_t> dstIndices;
                const uint32_t *srcPixels = img->data32;
                uint32_t maxIndex = 0;
                for (std::remove_const<decltype(nrOfPixels)>::type i = 0; i < nrOfPixels; i++)
                {
                    const auto index = PLUM_RED_32(*srcPixels++);
                    REQUIRE(index <= 255, std::runtime_error, "Bad index value: " << index);
                    maxIndex = maxIndex < index ? index : maxIndex;
                    dstIndices.emplace_back(static_cast<uint8_t>(index));
                }
                // build greyscale color map
                std::vector<Color::XRGB8888> dstColorMap;
                for (uint32_t i = 0; i <= maxIndex; ++i)
                {
                    dstColorMap.emplace_back(Color::XRGB8888(i, i, i));
                }
                return Image::ImageData(dstIndices, Color::Format::Paletted8, dstColorMap);
            }
            else
            {
                // read image data
                const auto nrOfPixels = img->width * img->height;
                std::vector<Color::XRGB8888> dstPixels;
                const uint32_t *srcPixels = img->data32;
                for (std::remove_const<decltype(nrOfPixels)>::type i = 0; i < nrOfPixels; i++)
                {
                    auto pixel = *srcPixels++;
                    dstPixels.emplace_back(Color::XRGB8888(PLUM_RED_32(pixel), PLUM_GREEN_32(pixel), PLUM_BLUE_32(pixel)));
                }
                return Image::ImageData(dstPixels);
            }
        }
        THROW(std::runtime_error, "Unsupported image type");
    }

    auto File::readImage(const std::string &filePath) -> Image::Frame
    {
        unsigned int error = PLUM_OK;
        plum_image *img = plum_load_image(filePath.c_str(), PLUM_MODE_FILENAME, PLUM_COLOR_32 | PLUM_ALPHA_INVERT | PLUM_PALETTE_LOAD, &error);
        if (img != nullptr && error == PLUM_OK)
        {
            REQUIRE(img->width > 0 && img->height > 0, std::runtime_error, "Bad image dimensions for \"" << filePath << "\"");
            Image::Frame data;
            data.type = Image::DataType::Flags::Bitmap;
            data.info.size = {img->width, img->height};
            data.data = getImageData(img);
            data.info.pixelFormat = data.data.pixels().format();
            plum_destroy_image(img);
            return data;
        }
        THROW(std::runtime_error, "Failed to read image \"" << filePath << "\":" << plum_get_error_text(error));
    }

    auto File::writeImage(const Image::Frame &src, const std::string &folder, const std::string &fileName) -> void
    {
        REQUIRE(src.data.pixels().format() != Color::Format::Unknown, std::runtime_error, "Bad color format");
        REQUIRE(src.info.size.width() > 0 && src.info.size.height() > 0, std::runtime_error, "Bad image size");
        REQUIRE(!src.fileName.empty() || !fileName.empty(), std::runtime_error, "Either image.fileName or fileName must contain a file name");
        // create libplum image
        plum_image dstImage{};
        dstImage.type = PLUM_IMAGE_PNG,
        dstImage.width = src.info.size.width();
        dstImage.height = src.info.size.height();
        dstImage.color_format = PLUM_COLOR_32;
        dstImage.max_palette_index = 0;
        dstImage.frames = 1;
        // create data storage
        std::vector<uint8_t> data8;   // index data if any
        std::vector<uint32_t> data32; // RGBA image data or color map if any
        // check if image is paletted
        if (src.data.colorMap().empty())
        {
            // true-color. get image pixels
            const auto srcPixels = src.data.pixels().convertData<Color::XRGB8888>();
            for (std::size_t i = 0; i < srcPixels.size(); ++i)
            {
                const auto pixel = srcPixels.at(i);
                data32.push_back(PLUM_COLOR_VALUE_32(pixel.R(), pixel.G(), pixel.B(), 0));
            }
            dstImage.data32 = data32.data();
        }
        else
        {
            // paletted. get image indices
            data8 = src.data.pixels().convertDataToRaw();
            dstImage.data8 = data8.data();
            // get image palette
            const auto srcColors = src.data.colorMap().convertData<Color::XRGB8888>();
            for (std::size_t i = 0; i < srcColors.size(); ++i)
            {
                const auto color = srcColors.at(i);
                data32.push_back(PLUM_COLOR_VALUE_32(color.R(), color.G(), color.B(), 0));
            }
            REQUIRE(data32.size() > 0, std::runtime_error, "Palette can not be empty");
            // check index data
            for (std::size_t i = 0; i < data8.size(); ++i)
            {
                REQUIRE(data8[i] < data32.size(), std::runtime_error, "Bad palette index " << data8[i] << " in pixel " << i);
            }
            dstImage.palette32 = data32.data();
            dstImage.max_palette_index = data32.size() - 1;
        }
        // check if we've created a valid image
        if (const auto error = plum_validate_image(&dstImage) != PLUM_OK)
        {
            THROW(std::runtime_error, "Failed to validate image:" << plum_get_error_text(error));
        }
        // create paths if neccessary
        auto outName = !fileName.empty() ? fileName : src.fileName;
        auto outPath = folder.empty() ? std::filesystem::path(outName) : std::filesystem::path(folder) / std::filesystem::path(outName).filename();
        if (!folder.empty() && !std::filesystem::exists(std::filesystem::path(folder)))
        {
            std::filesystem::create_directory(std::filesystem::path(folder));
        }
        // write to disk
        unsigned int error = PLUM_OK;
        const auto sizeWritten = plum_store_image(&dstImage, (void *)outPath.string().c_str(), PLUM_MODE_FILENAME, &error);
        if (sizeWritten == 0 || error != PLUM_OK)
        {
            THROW(std::runtime_error, "Failed to write image " << outPath << ": " << plum_get_error_text(error));
        }
    }

    auto File::writeImages(const std::vector<Image::Frame> &images, const std::string &folder) -> void
    {
        for (const auto &i : images)
        {
            REQUIRE(!i.fileName.empty(), std::runtime_error, "Image fileName can not be empty");
            writeImage(i, folder);
        }
    }

    auto File::writeRawImage(const Image::Frame &src, const std::string &folder, const std::string &fileName) -> void
    {
        REQUIRE(src.data.pixels().format() != Color::Format::Unknown, std::runtime_error, "Bad color format");
        REQUIRE(src.info.size.width() > 0 && src.info.size.height() > 0, std::runtime_error, "Bad image size");
        REQUIRE(!src.fileName.empty() || !fileName.empty(), std::runtime_error, "Either image.fileName or fileName must contain a file name");
        // create paths if neccessary
        auto outName = !fileName.empty() ? fileName : src.fileName;
        auto outPath = std::filesystem::path(folder) / std::filesystem::path(outName).filename();
        if (!std::filesystem::exists(std::filesystem::path(folder)))
        {
            std::filesystem::create_directory(std::filesystem::path(folder));
        }
        // write to disk
        auto ofs = std::ofstream(outPath, std::ios::binary);
        auto pixels = src.data.pixels().convertDataToRaw();
        ofs.write(reinterpret_cast<const char *>(pixels.data()), pixels.size());
    }
}

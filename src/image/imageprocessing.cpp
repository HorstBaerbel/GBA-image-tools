#include "imageprocessing.h"

#include "color/colorhelpers.h"
#include "color/gamma.h"
#include "color/optimizedistance.h"
#include "color/rgb565.h"
#include "color/xrgb1555.h"
#include "color/xrgb8888.h"
#include "compression/lz4.h"
#include "compression/lzss.h"
#include "compression/rans.h"
#include "exception.h"
#include "image/imageio.h"
#include "image_codec/dxt.h"
#include "imagehelpers.h"
#include "math/colorfit.h"
#include "processing/datahelpers.h"
#include "processing/varianthelpers.h"
#include "quantization.h"
#include "spritehelpers.h"
#include "video_codec/dxtv.h"
#include "video_codec/gvid.h"

#include <iomanip>
#include <iostream>
#include <memory>
// #include <filesystem>

namespace Image
{

    const std::map<ProcessingType, Processing::ProcessingFunc>
        Processing::ProcessingFunctions = {
            {ProcessingType::ConvertBlackWhite, {"binary", ConvertFunc(toBlackWhite)}},
            {ProcessingType::ConvertPaletted, {"paletted", ConvertFunc(toPaletted)}},
            {ProcessingType::ConvertTruecolor, {"truecolor", ConvertFunc(toTruecolor)}},
            {ProcessingType::ConvertCommonPalette, {"common palette", BatchConvertFunc(toCommonPalette)}},
            {ProcessingType::ConvertTiles, {"tiles", ConvertFunc(toTiles)}},
            {ProcessingType::ConvertSprites, {"sprites", ConvertFunc(toSprites)}},
            {ProcessingType::BuildTileMap, {"tilemap", ConvertFunc(toUniqueTileMap)}},
            {ProcessingType::AddColor0, {"add color #0", ConvertFunc(addColor0)}},
            {ProcessingType::MoveColor0, {"move color #0", ConvertFunc(moveColor0)}},
            {ProcessingType::ReorderColors, {"reorder colors", ConvertFunc(reorderColors)}},
            {ProcessingType::ShiftIndices, {"shift indices", ConvertFunc(shiftIndices)}},
            {ProcessingType::PruneIndices, {"prune indices", ConvertFunc(pruneIndices)}},
            {ProcessingType::ConvertDelta8, {"delta-8", ConvertFunc(toDelta8)}},
            {ProcessingType::ConvertDelta16, {"delta-16", ConvertFunc(toDelta16)}},
            {ProcessingType::DeltaImage, {"pixel diff", ConvertStateFunc(pixelDiff)}},
            {ProcessingType::CompressLZ4_40, {"compress LZ4 40h", ConvertFunc(compressLZ4_40)}},
            {ProcessingType::CompressLZSS_10, {"compress LZSS 10h", ConvertFunc(compressLZSS_10)}},
            {ProcessingType::CompressRANS_50, {"compress rANS 50h", ConvertFunc(compressRANS_50)}},
            //{ProcessingType::CompressRLE, {"compress RLE", ConvertFunc(compressRLE)}},
            {ProcessingType::CompressDXT, {"compress DXT", ConvertFunc(compressDXT)}},
            {ProcessingType::CompressDXTV, {"compress DXTV", ConvertStateFunc(compressDXTV)}},
            {ProcessingType::CompressGVID, {"compress GVID", ConvertStateFunc(compressGVID)}},
            {ProcessingType::ConvertPixelsToRaw, {"convert pixels", ConvertFunc(convertPixelsToRaw)}},
            {ProcessingType::PadPixelData, {"pad pixel data", ConvertFunc(padPixelData)}},
            {ProcessingType::PadColorMap, {"pad color map", ConvertFunc(padColorMap)}},
            {ProcessingType::EqualizeColorMaps, {"equalize color maps", BatchConvertFunc(equalizeColorMaps)}},
            {ProcessingType::ConvertColorMapToRaw, {"convert color map", ConvertFunc(convertColorMapToRaw)}},
            {ProcessingType::PadColorMapData, {"pad color map data", ConvertFunc(padColorMapData)}}};

    Frame Processing::toBlackWhite(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "toBlackWhite expects bitmaps as input data");
        REQUIRE(data.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "Expected RGB888 input data");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Quantization::Method, double>(parameters)), std::runtime_error, "toBlackWhite expects a Quantization::Method and double threshold parameter");
        const auto quantizationMethod = VariantHelpers::getValue<Quantization::Method, 0>(parameters);
        const auto threshold = VariantHelpers::getValue<double, 1>(parameters);
        REQUIRE(threshold >= 0 && threshold <= 1, std::runtime_error, "Threshold must be in [0.0, 1.0]");
        // threshold image
        auto result = data;
        result.data = Quantization::quantizeThreshold(data.data, static_cast<float>(threshold));
        REQUIRE(result.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted image");
        result.info.pixelFormat = result.data.pixels().format();
        result.info.colorMapFormat = result.data.colorMap().format();
        result.info.nrOfColorMapEntries = result.data.colorMap().size();
        return result;
    }

    Frame Processing::toPaletted(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "toPaletted expects bitmaps as input data");
        REQUIRE(data.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "toPaletted expects RGB888 input data");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Quantization::Method, uint32_t, std::vector<Color::XRGB8888>>(parameters)), std::runtime_error, "toPaletted expects a Quantization::Method, uint32_t number of colors parameter and a std::vector<Color::XRGB8888> color space map");
        const auto quantizationMethod = VariantHelpers::getValue<Quantization::Method, 0>(parameters);
        const auto nrOfColors = VariantHelpers::getValue<uint32_t, 1>(parameters);
        REQUIRE(nrOfColors >= 2 && nrOfColors <= 256, std::runtime_error, "Number of colors must be in [2, 256]");
        const auto colorSpaceMap = VariantHelpers::getValue<std::vector<Color::XRGB8888>, 2>(parameters);
        REQUIRE(colorSpaceMap.size() > 0, std::runtime_error, "colorSpaceMap can not be empty");
        // use cluster fit to find optimum color mapping
        ColorFit<Color::XRGB8888> colorFit(colorSpaceMap);
        const auto srcPixels = data.data.pixels().data<Color::XRGB8888>();
        const auto colorMapping = colorFit.reduceColors(srcPixels, nrOfColors);
        REQUIRE(colorMapping.size() > 0 && nrOfColors >= colorMapping.size(), std::runtime_error, "Unexpected number of mapped colors");
        // convert image to paletted possibly using dithering
        auto result = data;
        switch (quantizationMethod)
        {
        case Quantization::Method::ClosestColor:
            result.data = Quantization::quantizeClosest(data.data, colorMapping);
            break;
        case Quantization::Method::AtkinsonDither:
            result.data = Quantization::atkinsonDither(data.data, data.info.size.width(), data.info.size.height(), colorMapping);
            break;
        default:
            THROW(std::runtime_error, "Unsupported quantization method " << Quantization::toString(quantizationMethod));
        }
        REQUIRE(result.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted return image");
        result.info.pixelFormat = result.data.pixels().format();
        result.info.colorMapFormat = result.data.colorMap().format();
        result.info.nrOfColorMapEntries = result.data.colorMap().size();
        return result;
    }

    std::vector<Frame> Processing::toCommonPalette(const std::vector<Frame> &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.size() > 1, std::runtime_error, "toCommonPalette expects more than one input image");
        REQUIRE(data.front().type.isBitmap(), std::runtime_error, "toCommonPalette expects bitmaps as input data");
        REQUIRE(data.front().data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "toCommonPalette expects RGB888 input data");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Quantization::Method, uint32_t, std::vector<Color::XRGB8888>>(parameters)), std::runtime_error, "toCommonPalette expects a Quantization::Method, uint32_t number of colors parameter and a std::vector<Color::XRGB8888> color space map");
        const auto quantizationMethod = VariantHelpers::getValue<Quantization::Method, 0>(parameters);
        const auto nrOfColors = VariantHelpers::getValue<uint32_t, 1>(parameters);
        REQUIRE(nrOfColors >= 2 && nrOfColors <= 256, std::runtime_error, "Number of colors must be in [2, 256]");
        const auto colorSpaceMap = VariantHelpers::getValue<std::vector<Color::XRGB8888>, 2>(parameters);
        REQUIRE(colorSpaceMap.size() > 0, std::runtime_error, "colorSpaceMap can not be empty");
        ColorFit<Color::XRGB8888> colorFit(colorSpaceMap);
        std::cout << "Combining images..." << std::endl;
        // combine all images into one
        std::vector<Color::XRGB8888> combinedPixels;
        std::for_each(data.cbegin(), data.cend(), [&combinedPixels](const Frame &d)
                      { const auto srcPixels = d.data.pixels().data<Color::XRGB8888>();
                        combinedPixels.reserve(combinedPixels.size() + srcPixels.size());
                        std::copy(srcPixels.cbegin(), srcPixels.cend(), std::back_inserter(combinedPixels)); });
        // calculate common color map
        std::cout << "Building common color map (this might take some time)..." << std::endl;
        const auto colorMapping = colorFit.reduceColors(combinedPixels, nrOfColors);
        REQUIRE(colorMapping.size() > 0 && nrOfColors >= colorMapping.size(), std::runtime_error, "Unexpected number of mapped colors");
        // apply color map to all images
        std::cout << "Converting images..." << std::endl;
        std::vector<Frame> result(data.size());
#pragma omp parallel for
        for (int di = 0; di < static_cast<int>(data.size()); di++)
        {
            // convert image to paletted using dithering
            const auto &d = data.at(di);
            auto r = d;
            switch (quantizationMethod)
            {
            case Quantization::Method::ClosestColor:
                r.data = Quantization::quantizeClosest(d.data, colorMapping);
                break;
            case Quantization::Method::AtkinsonDither:
                r.data = Quantization::atkinsonDither(d.data, d.info.size.width(), d.info.size.height(), colorMapping);
                break;
            default:
                THROW(std::runtime_error, "Unsupported quantization method " << Quantization::toString(quantizationMethod));
            }
            REQUIRE(r.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted return image");
            // auto dumpPath = std::filesystem::current_path() / "result" / (std::to_string(d.index) + ".png");
            // temp.write(dumpPath.c_str());
            r.info.pixelFormat = r.data.pixels().format();
            r.info.colorMapFormat = r.data.colorMap().format();
            r.info.nrOfColorMapEntries = r.data.colorMap().size();
            result.at(di) = r;
        }
        return result;
    }

    Frame Processing::toTruecolor(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "toTruecolor expects bitmaps as input data");
        REQUIRE(data.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "toTruecolor expects a RGB888 image");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::Format>(parameters), std::runtime_error, "toTruecolor expects a Color::Format parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::RGB565 || format == Color::Format::XRGB8888, std::runtime_error, "Color format must be in [RGB555, RGB565, RGB888]");
        auto result = data;
        // convert colors if needed
        if (format == Color::Format::XRGB1555)
        {
            result.data = data.data.pixels().convertData<Color::XRGB1555>();
        }
        else if (format == Color::Format::RGB565)
        {
            result.data = data.data.pixels().convertData<Color::RGB565>();
        }
        result.info.pixelFormat = result.data.pixels().format();
        result.info.colorMapFormat = result.data.colorMap().format();
        result.info.nrOfColorMapEntries = 0;
        return result;
    }

    // ----------------------------------------------------------------------------

    Frame Processing::toUniqueTileMap(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap() && data.type.isTiles(), std::runtime_error, "toUniqueTileMap expects tiled bitmaps as input data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "toUniqueTileMap expects a bool detect flips parameter");
        const auto detectFlips = VariantHelpers::getValue<bool, 0>(parameters);
        auto result = data;
        auto screenAndTileMap = buildUniqueTileMap(data.data.pixels(), data.info.size.width(), data.info.size.height(), detectFlips);
        result.map.size = result.info.size;
        result.map.data = screenAndTileMap.first;
        result.data.pixels() = screenAndTileMap.second;
        result.type.setBitmap(false);
        return result;
    }

    Frame Processing::toTiles(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap() || data.type.isSprites(), std::runtime_error, "toTiles expects bitmaps or sprites as input data");
        auto result = data;
        result.data.pixels() = convertToTiles(data.data.pixels(), data.info.size.width(), data.info.size.height());
        result.type.setTiles();
        return result;
    }

    Frame Processing::toSprites(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap() || data.type.isTiles(), std::runtime_error, "toSprites expects bitmaps or tiles as input data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "toSprites expects a uint32_t sprite width parameter");
        const auto spriteWidth = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // convert image to sprites
        auto result = data;
        result.type.setSprites();
        if (data.info.size.width() != spriteWidth)
        {
            result.data.pixels() = convertToWidth(data.data.pixels(), data.info.size.width(), data.info.size.height(), spriteWidth);
            result.info.size = {spriteWidth, (data.info.size.width() * data.info.size.height()) / spriteWidth};
        }
        return result;
    }

    // ----------------------------------------------------------------------------

    Frame Processing::addColor0(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Adding a color can only be done for 8bit paletted images");
        REQUIRE(data.data.colorMap().format() == Color::Format::XRGB8888, std::runtime_error, "Adding a color can only be done for RGB888 color maps");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::XRGB8888>(parameters), std::runtime_error, "addColor0 expects a RGB888 color parameter");
        const auto color0 = VariantHelpers::getValue<Color::XRGB8888, 0>(parameters);
        // check for space in color map
        REQUIRE(data.data.colorMap().size() <= 255, std::runtime_error, "No space in color map (image has " << data.data.colorMap().size() << " colors)");
        // add color at front of color map
        auto result = data;
        result.data.pixels().data<uint8_t>() = ImageHelpers::incValuesBy1(data.data.pixels().data<uint8_t>());
        result.data.colorMap().data<Color::XRGB8888>() = ColorHelpers::addColorAtIndex0(data.data.colorMap().data<Color::XRGB8888>(), color0);
        result.info.nrOfColorMapEntries = result.data.colorMap().size();
        return result;
    }

    Frame Processing::moveColor0(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Moving a color can only be done for 8bit paletted images");
        REQUIRE(data.data.colorMap().format() == Color::Format::XRGB8888, std::runtime_error, "Moving a color can only be done for RGB888 color maps");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::XRGB8888>(parameters), std::runtime_error, "moveColor0 expects a RGB888 color parameter");
        const auto color0 = VariantHelpers::getValue<Color::XRGB8888, 0>(parameters);
        // try to find color in palette
        auto colorMap = data.data.colorMap().data<Color::XRGB8888>();
        auto oldColorIt = std::find(colorMap.begin(), colorMap.end(), color0);
        REQUIRE(oldColorIt != colorMap.end(), std::runtime_error, "Color " << color0.toHex() << " not found in image color map");
        const size_t oldIndex = std::distance(colorMap.begin(), oldColorIt);
        // check if index needs to move
        if (oldIndex != 0)
        {
            auto result = data;
            // move index in color map and pixel data
            std::swap(colorMap[oldIndex], colorMap[0]);
            result.data.colorMap().data<Color::XRGB8888>() = colorMap;
            result.data.pixels().data<uint8_t>() = ImageHelpers::swapValueWith0(data.data.pixels().data<uint8_t>(), oldIndex);
            return result;
        }
        return data;
    }

    Frame Processing::reorderColors(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Reordering colors can only be done for 8bit paletted images");
        REQUIRE(data.data.colorMap().format() == Color::Format::XRGB8888, std::runtime_error, "Reordering colors can only be done for RGB888 color maps");
        const auto newOrder = ColorHelpers::optimizeColorDistance(data.data.colorMap().data<Color::XRGB8888>());
        auto result = data;
        result.data.pixels().data<uint8_t>() = ImageHelpers::swapValues(data.data.pixels().data<uint8_t>(), newOrder);
        result.data.colorMap().data<Color::XRGB8888>() = ColorHelpers::swapColors(data.data.colorMap().data<Color::XRGB8888>(), newOrder);
        return result;
    }

    Frame Processing::shiftIndices(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Shifting indices can only be done for 8bit paletted images");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "shiftIndices expects a uint32_t shift parameter");
        const auto shiftBy = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // find max. index value
        auto &dataIndices = data.data.pixels().data<uint8_t>();
        auto maxIndex = *std::max_element(dataIndices.cbegin(), dataIndices.cend());
        REQUIRE(maxIndex + shiftBy <= 255, std::runtime_error, "Max. index value in image is " << maxIndex << ", shift is " << shiftBy << "! Resulting index values would be > 255");
        // shift indices
        Frame result = data;
        auto &resultIndices = result.data.pixels().data<uint8_t>();
        std::for_each(resultIndices.begin(), resultIndices.end(), [shiftBy](auto &index)
                      { index = (index == 0) ? 0 : (((index + shiftBy) > 255) ? 255 : (index + shiftBy)); });
        return result;
    }

    Frame Processing::pruneIndices(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Index pruning only possible for 8bit paletted images");
        REQUIRE(data.data.colorMap().size() <= 16, std::runtime_error, "Index pruning only possible for images with <= 16 colors");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "pruneIndices expects a uint32_t bit depth parameter");
        const auto bitDepth = VariantHelpers::getValue<uint32_t, 0>(parameters);
        REQUIRE(bitDepth == 1 || bitDepth == 2 || bitDepth == 4, std::runtime_error, "Bit depth must be in [1, 2, 4]");
        auto result = data;
        auto &indices = result.data.pixels().data<uint8_t>();
        auto maxIndex = *std::max_element(indices.cbegin(), indices.cend());
        if (bitDepth == 1)
        {
            REQUIRE(maxIndex == 1, std::runtime_error, "Index pruning to 1 bit only possible with index data <= 1");
            result.data.pixels() = PixelData(ImageHelpers::convertDataTo1Bit(indices), Color::Format::Paletted1);
        }
        else if (bitDepth == 2)
        {
            REQUIRE(maxIndex < 4, std::runtime_error, "Index pruning to 2 bit only possible with index data <= 3");
            result.data.pixels() = PixelData(ImageHelpers::convertDataTo2Bit(indices), Color::Format::Paletted2);
        }
        else
        {
            REQUIRE(maxIndex < 16, std::runtime_error, "Index pruning to 4 bit only possible with index data <= 15");
            result.data.pixels() = PixelData(ImageHelpers::convertDataTo4Bit(indices), Color::Format::Paletted4);
        }
        result.info.pixelFormat = result.data.pixels().format();
        return result;
    }

    Frame Processing::toDelta8(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        auto result = data;
        result.data.pixels() = PixelData(DataHelpers::deltaEncode(result.data.pixels().convertDataToRaw()), Color::Format::Unknown);
        result.type.setCompressed();
        return result;
    }

    Frame Processing::toDelta16(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        auto result = data;
        result.data.pixels() = PixelData(DataHelpers::convertTo<uint8_t>(DataHelpers::deltaEncode(DataHelpers::convertTo<uint16_t>(result.data.pixels().convertDataToRaw()))), Color::Format::Unknown);
        result.type.setCompressed();
        return result;
    }

    // ----------------------------------------------------------------------------

    Frame Processing::compressLZ4_40(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "compressLZ4_40 expects a bool VRAMcompatible parameter");
        const auto vramCompatible = VariantHelpers::getValue<bool, 0>(parameters);
        // compress data
        auto result = data;
        result.data.pixels() = PixelData(Compression::encodeLZ4_40(result.data.pixels().convertDataToRaw(), vramCompatible), Color::Format::Unknown);
        result.type.setCompressed();
        // print statistics
        if (statistics != nullptr)
        {
            const auto ratioPercent = static_cast<double>(result.data.pixels().rawSize() * 100.0 / static_cast<double>(data.data.pixels().rawSize()));
            std::cout << "LZ4 40h compression ratio: " << std::fixed << std::setprecision(1) << ratioPercent << "%" << std::endl;
        }
        return result;
    }

    Frame Processing::compressLZSS_10(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "compressLZSS_10 expects a bool VRAMcompatible parameter");
        const auto vramCompatible = VariantHelpers::getValue<bool, 0>(parameters);
        // compress data
        auto result = data;
        result.data.pixels() = PixelData(Compression::encodeLZSS_10(result.data.pixels().convertDataToRaw(), vramCompatible), Color::Format::Unknown);
        result.type.setCompressed();
        // print statistics
        if (statistics != nullptr)
        {
            const auto ratioPercent = static_cast<double>(result.data.pixels().rawSize() * 100.0 / static_cast<double>(data.data.pixels().rawSize()));
            std::cout << "LZSS 10h compression ratio: " << std::fixed << std::setprecision(1) << ratioPercent << "%" << std::endl;
        }
        return result;
    }

    Frame Processing::compressRANS_50(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // compress data
        auto result = data;
        result.data.pixels() = PixelData(Compression::encodeRANS_50(result.data.pixels().convertDataToRaw()), Color::Format::Unknown);
        result.type.setCompressed();
        // print statistics
        if (statistics != nullptr)
        {
            const auto ratioPercent = static_cast<double>(result.data.pixels().rawSize() * 100.0 / static_cast<double>(data.data.pixels().rawSize()));
            std::cout << "rANS 50h compression ratio: " << std::fixed << std::setprecision(1) << ratioPercent << "%" << std::endl;
        }
        return result;
    }

    Frame Processing::compressRLE(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "compressRLE expects a bool VRAMcompatible parameter");
        const auto vramCompatible = VariantHelpers::getValue<bool, 0>(parameters);
        // compress data
        auto result = data;
        // result.data = RLE::encodeRLE(image.data, vramCompatible);
        result.type.setCompressed();
        return result;
    }

    Frame Processing::compressDXT(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "compressDXT expects bitmaps as input data");
        REQUIRE(data.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "DXT compression is only possible for RGB888 truecolor images");
        REQUIRE(data.info.size.width() % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
        REQUIRE(data.info.size.height() % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT compression");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::Format>(parameters), std::runtime_error, "compressDXT expects a Color::Format parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::RGB565 ||
                    format == Color::Format::XBGR1555 || format == Color::Format::BGR565,
                std::runtime_error, "Output color format must be in [RGB555, RGB565, BGR555, BGR565]");
        // convert image using DXT compression
        auto result = data;
        auto compressedData = DXT::encode(data.data.pixels().data<Color::XRGB8888>(), data.info.size.width(), data.info.size.height(), format == Color::Format::RGB565, format == Color::Format::XBGR1555 || format == Color::Format::BGR565);
        result.data.pixels() = PixelData(compressedData, Color::Format::Unknown);
        result.info.pixelFormat = format;
        result.info.colorMapFormat = Color::Format::Unknown;
        result.type.setCompressed();
        return result;
    }

    Frame Processing::compressDXTV(const Frame &data, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "compressDXTV expects bitmaps as input data");
        REQUIRE(data.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "DXTV compression is only possible for RGB888 truecolor images");
        REQUIRE(data.info.size.width() % 8 == 0, std::runtime_error, "Image width must be a multiple of 8 for DXTV compression");
        REQUIRE(data.info.size.height() % 8 == 0, std::runtime_error, "Image height must be a multiple of 8 for DXTV compression");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Color::Format, double>(parameters)), std::runtime_error, "compressDXTV expects a Color::Format and a double quality parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::XBGR1555, std::runtime_error, "Output color format must be in [RGB555, BGR555]");
        auto quality = VariantHelpers::getValue<double, 1>(parameters);
        REQUIRE(quality >= 0 && quality <= 100, std::runtime_error, "compressDXTV quality must be in [0, 100]");
        // convert image using DXTV compression
        auto result = data;
        auto previousImage = state.empty() ? std::vector<Color::XRGB8888>() : DataHelpers::convertTo<Color::XRGB8888>(state);
        auto compressedData = Video::Dxtv::encode(data.data.pixels().data<Color::XRGB8888>(), previousImage, data.info.size.width(), data.info.size.height(), quality, format == Color::Format::XBGR1555, statistics);
        result.data.pixels() = PixelData(compressedData.first, Color::Format::Unknown);
        result.info.pixelFormat = format;
        result.info.colorMapFormat = Color::Format::Unknown;
        result.type.setCompressed();
        // store decompressed image as state
        state = DataHelpers::convertTo<uint8_t>(compressedData.second);
        // add statistics
        if (statistics != nullptr)
        {
            statistics->setImage("DXTV output", state, Color::Format::XRGB8888, result.info.size.width(), result.info.size.height());
        }
        return result;
    }

    Frame Processing::compressGVID(const Frame &data, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "compressGVID expects bitmaps as input data");
        REQUIRE(data.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "GVID compression is only possible for RGB888 truecolor images");
        REQUIRE(data.info.size.width() % 16 == 0, std::runtime_error, "Image width must be a multiple of 16 for GVID compression");
        REQUIRE(data.info.size.height() % 16 == 0, std::runtime_error, "Image height must be a multiple of 16 for GVID compression");
        auto result = data;
        auto compressedData = GVID::encodeGVID(data.data.pixels().data<Color::XRGB8888>(), data.info.size.width(), data.info.size.height());
        result.data.pixels() = PixelData(compressedData, Color::Format::Unknown);
        result.info.pixelFormat = Color::Format::YCgCoRf;
        result.info.colorMapFormat = Color::Format::Unknown;
        result.type.setCompressed();
        return result;
    }

    // ----------------------------------------------------------------------------

    Frame Processing::convertPixelsToRaw(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::Format>(parameters), std::runtime_error, "convertPixelsToRaw expects a Color::Format parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::RGB565 || format == Color::Format::XRGB8888 ||
                    format == Color::Format::XBGR1555 || format == Color::Format::BGR565 || format == Color::Format::XBGR8888,
                std::runtime_error, "Color format must be in [RGB555, RGB565, RGB888, BGR555, BGR565, BGR888]");
        // if pixel data is already raw return it
        if (data.data.pixels().isRaw())
        {
            return data;
        }
        auto result = data;
        // if pixel data is indexed we don't need to convert the color format
        if (data.data.pixels().isIndexed())
        {
            result.data.pixels() = PixelData(result.data.pixels().convertDataToRaw(), Color::Format::Unknown);
        }
        else
        {
            result.data.pixels() = PixelData(result.data.pixels().convertTo(format).convertDataToRaw(), Color::Format::Unknown);
            result.info.pixelFormat = format;
        }
        return result;
    }

    Frame Processing::padPixelData(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.data.pixels().isRaw(), std::runtime_error, "Pixel data padding is only possible for raw data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "padPixelData expects a uint32_t pad modulo parameter");
        auto multipleOf = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // pad pixel data
        auto result = data;
        result.data.pixels() = PixelData(DataHelpers::fillUpToMultipleOf(data.data.pixels().convertDataToRaw(), multipleOf), Color::Format::Unknown);
        return result;
    }

    Frame Processing::convertColorMapToRaw(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::Format>(parameters), std::runtime_error, "convertColorMapToRaw expects a Color::Format parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::RGB565 || format == Color::Format::XRGB8888 ||
                    format == Color::Format::XBGR1555 || format == Color::Format::BGR565 || format == Color::Format::XBGR8888,
                std::runtime_error, "Color format must be in [RGB555, RGB565, RGB888, BGR555, BGR565, BGR888]");
        REQUIRE(!data.data.colorMap().empty(), std::runtime_error, "Color map can not be empty");
        // if color map data is already raw return it
        if (data.data.colorMap().isRaw())
        {
            return data;
        }
        auto result = data;
        result.data.colorMap() = PixelData(result.data.colorMap().convertTo(format).convertDataToRaw(), Color::Format::Unknown);
        result.info.colorMapFormat = format;
        return result;
    }

    Frame Processing::padMapData(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(!data.map.data.empty(), std::runtime_error, "Map data can not be empty");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "padMapData expects a uint32_t pad modulo parameter");
        auto multipleOf = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // pad map data
        auto result = data;
        result.map.data = DataHelpers::fillUpToMultipleOf(data.map.data, multipleOf);
        return result;
    }

    Frame Processing::padColorMap(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "padColorMap expects a uint32_t pad modulo parameter");
        auto multipleOf = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // pad data
        auto result = data;
        result.data.colorMap() = std::visit([multipleOf, format = data.data.colorMap().format()](auto colorMap) -> PixelData
                                            { 
                using T = std::decay_t<decltype(colorMap)>;
                if constexpr (std::is_same<T, std::vector<Color::XRGB1555>>() || std::is_same<T, std::vector<Color::RGB565>>() || std::is_same<T, std::vector<Color::XRGB8888>>())
                {
                    return PixelData(DataHelpers::fillUpToMultipleOf(colorMap, multipleOf), format);
                }
                THROW(std::runtime_error, "Color format must be XRGB1555, RGB565 or XRGB8888"); },
                                            data.data.colorMap().storage());
        result.info.nrOfColorMapEntries = result.data.colorMap().size();
        return result;
    }

    Frame Processing::padColorMapData(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.data.colorMap().isRaw(), std::runtime_error, "Color map data padding is only possible for raw data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "padColorMapData expects a single uint32_t pad modulo parameter");
        auto multipleOf = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // pad raw color map data
        auto result = data;
        result.data.colorMap() = PixelData(DataHelpers::fillUpToMultipleOf(data.data.colorMap().convertDataToRaw(), multipleOf), Color::Format::Unknown);
        return result;
    }

    std::vector<Frame> Processing::equalizeColorMaps(const std::vector<Frame> &images, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        auto allColorMapsSameSize = std::find_if_not(images.cbegin(), images.cend(), [refSize = images.front().data.colorMap().size()](const auto &img)
                                                     { return img.data.colorMap().size() == refSize; }) == images.cend();
        // pad color map if necessary
        if (!allColorMapsSameSize)
        {
            uint32_t maxColorMapColors = std::max_element(images.cbegin(), images.cend(), [](const auto &imgA, const auto &imgB)
                                                          { return imgA.data.colorMap().size() < imgB.data.colorMap().size(); })
                                             ->data.colorMap()
                                             .size();
            std::vector<Frame> result;
            std::transform(images.begin(), images.end(), std::back_inserter(result), [maxColorMapColors, statistics](auto &img)
                           { return padColorMap(img, {Parameter(maxColorMapColors)}, statistics); });
            return result;
        }
        return images;
    }

    Frame Processing::pixelDiff(const Frame &data, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Frame::SPtr statistics)
    {
        // check if a usable state was passed
        if (!state.empty())
        {
            auto result = data;
            // calculate difference and set state
            result.data.pixels() = std::visit([&state, format = data.data.pixels().format()](auto currentPixels) -> PixelData
                                              { 
                using T = std::decay_t<decltype(currentPixels)>;
                if constexpr(std::is_same<T, std::vector<uint8_t>>())
                {
                    auto & prevPixels = state;
                    std::vector<uint8_t> diff(currentPixels.size());
                    for (std::size_t i = 0; i < currentPixels.size(); i++)
                    {
                        diff[i] = prevPixels[i] - currentPixels[i];
                    }
                    state = diff;
                    return PixelData(diff, format);
                }
                else if constexpr (std::is_same<T, std::vector<Color::XRGB1555>>() || std::is_same<T, std::vector<Color::RGB565>>() || std::is_same<T, std::vector<Color::XRGB8888>>())
                {
                    auto prevPixels = DataHelpers::convertTo<typename T::value_type::pixel_type>(state);
                    std::vector<typename T::value_type::pixel_type> diff(currentPixels.size());
                    for (std::size_t i = 0; i < currentPixels.size(); i++)
                    {
                        diff[i] = prevPixels[i] - typename T::value_type::pixel_type(currentPixels[i]);
                    }
                    state = DataHelpers::convertTo<uint8_t>(diff);
                    return PixelData(DataHelpers::convertTo<typename T::value_type>(diff), format);
                }
                THROW(std::runtime_error, "Color format must be Paletted8, XRGB1555, RGB565 or XRGB8888"); },
                                              data.data.pixels().storage());
            return result;
        }
        // set current image to state
        state = data.data.pixels().convertDataToRaw();
        // no state, return input data
        return data;
    }

    // ----------------------------------------------------------------------------

    void Processing::dumpImage(const Frame &data, [[maybe_unused]] const std::vector<Parameter> &parameters, [[maybe_unused]] Statistics::Frame::SPtr statistics)
    {
        IO::File::writeImage(data, "", "result" + std::to_string(data.index) + ".png");
    }

    // ----------------------------------------------------------------------------

    void Processing::addStep(ProcessingType type, std::vector<Parameter> parameters, bool prependProcessingInfo, bool addStatistics)
    {
        // find function for type
        const auto pfIt = ProcessingFunctions.find(type);
        REQUIRE(pfIt != ProcessingFunctions.cend(), std::runtime_error, "Failed to find function for image processing type " << static_cast<uint32_t>(type));
        m_steps.push_back({type, parameters, prependProcessingInfo, addStatistics, {}, pfIt->second});
    }

    void Processing::addDumpStep(std::vector<Parameter> parameters, bool addStatistics)
    {
        m_steps.push_back({ProcessingType::Invalid, parameters, false, addStatistics, {}, {"dump", OutputFunc(dumpImage)}});
    }

    std::size_t Processing::nrOfSteps() const
    {
        return m_steps.size();
    }

    void Processing::clearSteps()
    {
        reset();
        m_steps.clear();
    }

    void Processing::reset()
    {
        for (std::size_t si = 0; si < m_steps.size(); si++)
        {
            m_steps[si].state.clear();
        }
    }

    std::string Processing::getProcessingDescription(const std::string &seperator)
    {
        std::string result;
        for (std::size_t si = 0; si < m_steps.size(); si++)
        {
            const auto &step = m_steps[si];
            const auto &stepFunc = step.function;
            result += stepFunc.description;
            result += step.parameters.size() > 0 ? " " : "";
            for (std::size_t pi = 0; pi < step.parameters.size(); pi++)
            {
                const auto &p = step.parameters[pi];
                if (std::holds_alternative<bool>(p))
                {
                    result += std::get<bool>(p) ? "true" : "false";
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<int32_t>(p))
                {
                    result += std::to_string(std::get<int32_t>(p));
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<uint32_t>(p))
                {
                    result += std::to_string(std::get<uint32_t>(p));
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<double>(p))
                {
                    result += std::to_string(std::get<double>(p));
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<Color::XRGB8888>(p))
                {
                    result += Color::XRGB8888::toHex(std::get<Color::XRGB8888>(p));
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<Color::Format>(p))
                {
                    result += Color::formatInfo(std::get<Color::Format>(p)).name;
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<std::string>(p))
                {
                    result += std::get<std::string>(p);
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
            }
            result += (si < (m_steps.size() - 1) ? seperator : "");
        }
        return result;
    }

    std::vector<Frame> Processing::processBatch(const std::vector<Frame> &data)
    {
        REQUIRE(data.size() > 0, std::runtime_error, "Empty data passed to processing");
        bool finalStepFound = false;
        std::vector<Frame> processed = data;
        for (auto stepIt = m_steps.begin(); stepIt != m_steps.end(); ++stepIt)
        {
            const auto &stepFunc = stepIt->function.func;
            // check if this was the final processing step (first non-input processing)
            bool isFinalStep = false;
            if (!finalStepFound)
            {
                isFinalStep = true;
                finalStepFound = true;
            }
            // process depending on operation type
            if (std::holds_alternative<ConvertFunc>(stepFunc))
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc);
                for (auto &img : processed)
                {
                    const auto inputSize = img.data.pixels().rawSize();
                    img = convertFunc(img, stepIt->parameters, nullptr);
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : img.data.pixels().rawSize() + sizeof(uint32_t);
                    img.info.maxMemoryNeeded = (img.info.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : img.info.maxMemoryNeeded;
                }
            }
            else if (std::holds_alternative<ConvertStateFunc>(stepFunc))
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc);
                for (auto &img : processed)
                {
                    const uint32_t inputSize = img.data.pixels().rawSize();
                    img = convertFunc(img, stepIt->parameters, stepIt->state, nullptr);
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : img.data.pixels().rawSize() + sizeof(uint32_t);
                    img.info.maxMemoryNeeded = (img.info.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : img.info.maxMemoryNeeded;
                }
            }
            else if (std::holds_alternative<BatchConvertFunc>(stepFunc))
            {
                // get all input sizes
                std::vector<uint32_t> inputSizes = {};
                std::transform(processed.cbegin(), processed.cend(), std::back_inserter(inputSizes), [](const auto &d)
                               { return d.data.pixels().rawSize(); });
                auto batchFunc = std::get<BatchConvertFunc>(stepFunc);
                processed = batchFunc(processed, stepIt->parameters, nullptr);
                for (auto pIt = processed.begin(); pIt != processed.end(); pIt++)
                {
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : pIt->data.pixels().rawSize() + sizeof(uint32_t);
                    pIt->info.maxMemoryNeeded = (pIt->info.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : pIt->info.maxMemoryNeeded;
                }
            }
            else if (std::holds_alternative<ReduceFunc>(stepFunc))
            {
                auto reduceFunc = std::get<ReduceFunc>(stepFunc);
                processed = {reduceFunc(processed, stepIt->parameters, nullptr)};
            }
            else if (std::holds_alternative<OutputFunc>(stepFunc))
            {
                auto outputFunc = std::get<OutputFunc>(stepFunc);
                for (auto pIt = processed.cbegin(); pIt != processed.cend(); pIt++)
                {
                    outputFunc(*pIt, stepIt->parameters, nullptr);
                }
            }
        }
        return processed;
    }

    Frame Processing::processStream(const Frame &data, Statistics::Container::SPtr statistics)
    {
        bool finalStepFound = false;
        Frame processed = data;
        auto frameStatistics = statistics != nullptr ? statistics->addFrame() : nullptr;
        for (auto stepIt = m_steps.begin(); stepIt != m_steps.end(); ++stepIt)
        {
            const uint32_t inputSize = processed.data.pixels().rawSize();
            auto stepStatistics = stepIt->addStatistics ? frameStatistics : nullptr;
            auto &stepFunc = stepIt->function.func;
            bool updateMaxMemoryNeeded = false;
            if (std::holds_alternative<ConvertFunc>(stepFunc))
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc);
                processed = convertFunc(processed, stepIt->parameters, stepStatistics);
                updateMaxMemoryNeeded = true;
            }
            else if (std::holds_alternative<ConvertStateFunc>(stepFunc))
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc);
                processed = convertFunc(processed, stepIt->parameters, stepIt->state, stepStatistics);
                updateMaxMemoryNeeded = true;
            }
            else if (std::holds_alternative<OutputFunc>(stepFunc))
            {
                auto outputFunc = std::get<OutputFunc>(stepFunc);
                outputFunc(processed, stepIt->parameters, stepStatistics);
            }
            // we're silently ignoring OperationType::BatchConvert and ::Reduce operations here
            if (updateMaxMemoryNeeded)
            {
                // record max. memory needed for everything, but the first step
                auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : processed.data.pixels().rawSize() + sizeof(uint32_t);
                processed.info.maxMemoryNeeded = (processed.info.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : processed.info.maxMemoryNeeded;
            }
        }
        return processed;
    }

    std::vector<ProcessingType> Processing::getDecodingSteps() const
    {
        std::vector<ProcessingType> decodingSteps;
        auto srIt = m_steps.crbegin();
        while (srIt != m_steps.crend())
        {
            REQUIRE(srIt->type != ProcessingType::Invalid, std::runtime_error, "Bad processing type for step " << m_steps.size() - static_cast<int32_t>(std::distance(m_steps.crend(), srIt)));
            if (srIt->decodeRelevant)
            {
                decodingSteps.push_back(srIt->type);
            }
            ++srIt;
        }
        return decodingSteps;
    }
}

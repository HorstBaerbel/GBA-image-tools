#include "imageprocessing.h"

#include "codec/dxt.h"
#include "codec/dxtv.h"
#include "codec/gvid.h"
#include "color/colorhelpers.h"
#include "color/optimizedistance.h"
#include "color/rgb565.h"
#include "color/xrgb1555.h"
#include "color/xrgb8888.h"
#include "compression/lzss.h"
#include "datahelpers.h"
#include "exception.h"
#include "imagehelpers.h"
#include "math/colorfit.h"
#include "quantization.h"
#include "spritehelpers.h"
#include "varianthelpers.h"

#include <iostream>
#include <memory>
// #include <filesystem>

namespace Image
{

    const std::map<ProcessingType, Processing::ProcessingFunc>
        Processing::ProcessingFunctions = {
            {ProcessingType::ConvertBlackWhite, {"binary", OperationType::Convert, FunctionType(toBlackWhite)}},
            {ProcessingType::ConvertPaletted, {"paletted", OperationType::Convert, FunctionType(toPaletted)}},
            {ProcessingType::ConvertCommonPalette, {"common palette", OperationType::BatchConvert, FunctionType(toCommonPalette)}},
            {ProcessingType::ConvertTruecolor, {"truecolor", OperationType::Convert, FunctionType(toTruecolor)}},
            {ProcessingType::BuildTileMap, {"tilemap", OperationType::Convert, FunctionType(toUniqueTileMap)}},
            {ProcessingType::ConvertTiles, {"tiles", OperationType::Convert, FunctionType(toTiles)}},
            {ProcessingType::ConvertSprites, {"sprites", OperationType::Convert, FunctionType(toSprites)}},
            {ProcessingType::AddColor0, {"add color #0", OperationType::Convert, FunctionType(addColor0)}},
            {ProcessingType::MoveColor0, {"move color #0", OperationType::Convert, FunctionType(moveColor0)}},
            {ProcessingType::ReorderColors, {"reorder colors", OperationType::Convert, FunctionType(reorderColors)}},
            {ProcessingType::ShiftIndices, {"shift indices", OperationType::Convert, FunctionType(shiftIndices)}},
            {ProcessingType::PruneIndices, {"prune indices", OperationType::Convert, FunctionType(pruneIndices)}},
            {ProcessingType::ConvertDelta8, {"delta-8", OperationType::Convert, FunctionType(toDelta8)}},
            {ProcessingType::ConvertDelta16, {"delta-16", OperationType::Convert, FunctionType(toDelta16)}},
            {ProcessingType::CompressLZ10, {"compress LZ10", OperationType::Convert, FunctionType(compressLZ10)}},
            //{ProcessingType::CompressRLE, {"compress RLE", OperationType::Convert, FunctionType(compressRLE)}},
            {ProcessingType::CompressDXT, {"compress DXT", OperationType::Convert, FunctionType(compressDXT)}},
            {ProcessingType::CompressDXTV, {"compress DXTV", OperationType::ConvertState, FunctionType(compressDXTV)}},
            {ProcessingType::CompressGVID, {"compress GVID", OperationType::ConvertState, FunctionType(compressGVID)}},
            {ProcessingType::PadPixelData, {"pad pixel data", OperationType::Convert, FunctionType(padPixelData)}},
            {ProcessingType::ConvertPixelsToRaw, {"convert pixels", OperationType::Convert, FunctionType(convertPixelsToRaw)}},
            {ProcessingType::PadColorMapData, {"pad color map data", OperationType::Convert, FunctionType(padColorMapData)}},
            {ProcessingType::ConvertColorMapToRaw, {"convert color map", OperationType::Convert, FunctionType(convertColorMapToRaw)}},
            {ProcessingType::PadColorMap, {"pad color map", OperationType::Convert, FunctionType(padColorMap)}},
            {ProcessingType::EqualizeColorMaps, {"equalize color maps", OperationType::BatchConvert, FunctionType(equalizeColorMaps)}},
            {ProcessingType::DeltaImage, {"pixel diff", OperationType::ConvertState, FunctionType(pixelDiff)}}};

    Data Processing::toBlackWhite(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "toBlackWhite expects bitmaps as input data");
        REQUIRE(data.image.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "Expected RGB888 input data");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Quantization::Method, double>(parameters)), std::runtime_error, "toBlackWhite expects a Quantization::Method and double threshold parameter");
        const auto quantizationMethod = VariantHelpers::getValue<Quantization::Method, 0>(parameters);
        const auto threshold = VariantHelpers::getValue<double, 1>(parameters);
        REQUIRE(threshold >= 0 && threshold <= 1, std::runtime_error, "Threshold must be in [0.0, 1.0]");
        // threshold image
        auto result = data;
        result.image.data = Quantization::quantizeThreshold(data.image.data, static_cast<float>(threshold));
        REQUIRE(result.image.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted image");
        result.image.pixelFormat = result.image.data.pixels().format();
        result.image.colorMapFormat = result.image.data.colorMap().format();
        result.image.nrOfColorMapEntries = result.image.data.colorMap().size();
        return result;
    }

    Data Processing::toPaletted(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "toPaletted expects bitmaps as input data");
        REQUIRE(data.image.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "toPaletted expects RGB888 input data");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Quantization::Method, uint32_t, std::vector<Color::XRGB8888>>(parameters)), std::runtime_error, "toPaletted expects a Quantization::Method, uint32_t number of colors parameter and a std::vector<Color::XRGB8888> color space map");
        const auto quantizationMethod = VariantHelpers::getValue<Quantization::Method, 0>(parameters);
        const auto nrOfColors = VariantHelpers::getValue<uint32_t, 1>(parameters);
        REQUIRE(nrOfColors >= 2 && nrOfColors <= 256, std::runtime_error, "Number of colors must be in [2, 256]");
        const auto colorSpaceMap = VariantHelpers::getValue<std::vector<Color::XRGB8888>, 2>(parameters);
        REQUIRE(colorSpaceMap.size() > 0, std::runtime_error, "colorSpaceMap can not be empty");
        // convert image to paletted possibly using dithering
        auto result = data;
        switch (quantizationMethod)
        {
        case Quantization::Method::ClosestColor:
            result.image.data = Quantization::quantizeClosest(data.image.data, nrOfColors, colorSpaceMap);
            break;
        case Quantization::Method::AtkinsonDither:
            result.image.data = Quantization::atkinsonDither(data.image.data, nrOfColors, colorSpaceMap);
            break;
        default:
            THROW(std::runtime_error, "Unsupported quantization method " << Quantization::toString(quantizationMethod));
        }
        REQUIRE(result.image.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted return image");
        result.image.pixelFormat = result.image.data.pixels().format();
        result.image.colorMapFormat = result.image.data.colorMap().format();
        result.image.nrOfColorMapEntries = result.image.data.colorMap().size();
        return result;
    }

    std::vector<Data> Processing::toCommonPalette(const std::vector<Data> &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.size() > 1, std::runtime_error, "toCommonPalette expects more than one input image");
        REQUIRE(data.front().type.isBitmap(), std::runtime_error, "toCommonPalette expects bitmaps as input data");
        REQUIRE(data.front().image.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "toCommonPalette expects RGB888 input data");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Quantization::Method, uint32_t, std::vector<Color::XRGB8888>>(parameters)), std::runtime_error, "toCommonPalette expects a Quantization::Method, uint32_t number of colors parameter and a std::vector<Color::XRGB8888> color space map");
        const auto quantizationMethod = VariantHelpers::getValue<Quantization::Method, 0>(parameters);
        const auto nrOfColors = VariantHelpers::getValue<uint32_t, 1>(parameters);
        REQUIRE(nrOfColors >= 2 && nrOfColors <= 256, std::runtime_error, "Number of colors must be in [2, 256]");
        const auto colorSpaceMap = VariantHelpers::getValue<std::vector<Color::XRGB8888>, 2>(parameters);
        REQUIRE(colorSpaceMap.size() > 0, std::runtime_error, "colorSpaceMap can not be empty");
        // build histogram of colors used in all input images
        std::cout << "Building histogram..." << std::endl;
        std::map<Color::XRGB8888, uint64_t> histogram;
        std::for_each(data.cbegin(), data.cend(), [&histogram](const auto &d)
                      {
            auto & imageData = d.image.data.pixels().template data<Color::XRGB8888>();
            std::for_each(imageData.cbegin(), imageData.cend(), [&histogram](auto pixel)
                { histogram[pixel]++; }); });
        std::cout << histogram.size() << " unique colors in " << data.size() << " images" << std::endl;
        // create as many preliminary clusters as colors in colorSpaceMap
        ColorFit<Color::XRGB8888> colorFit(colorSpaceMap);
        std::cout << "Color space has " << colorSpaceMap.size() << " colors" << std::endl;
        // sort histogram colors into closest clusters in parallel
        std::cout << "Sorting colors into max. " << nrOfColors << " clusters... (this might take some time)" << std::endl;
        std::vector<Color::XRGB8888> commonColorMap = {}; // colorFit.reduceColors(histogram, nrOfColors);
        // apply color map to all images
        std::cout << "Converting images..." << std::endl;
        std::vector<Data> result;
        std::transform(data.cbegin(), data.cend(), std::back_inserter(result), [&commonColorMap, quantizationMethod, nrOfColors](const auto &d)
                       {
                           // convert image to paletted using dithering
                           auto r = d;
                           switch (quantizationMethod)
                           {
                           case Quantization::Method::ClosestColor:
                               r.image.data = Quantization::quantizeClosest(d.image.data, nrOfColors, commonColorMap);
                               break;
                           case Quantization::Method::AtkinsonDither:
                               r.image.data = Quantization::atkinsonDither(d.image.data, nrOfColors, commonColorMap);
                               break;
                           default:
                               THROW(std::runtime_error, "Unsupported quantization method " << Quantization::toString(quantizationMethod));
                           }
                           REQUIRE(r.image.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted return image");
                           // auto dumpPath = std::filesystem::current_path() / "result" / (std::to_string(d.index) + ".png");
                           // temp.write(dumpPath.c_str());
                           r.image.pixelFormat = r.image.data.pixels().format();
                           r.image.colorMapFormat = r.image.data.colorMap().format();
                           r.image.nrOfColorMapEntries = r.image.data.colorMap().size();
                           return r; });
        return result;
    }

    Data Processing::toTruecolor(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "toTruecolor expects bitmaps as input data");
        REQUIRE(data.image.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "toTruecolor expects a RGB888 image");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::Format>(parameters), std::runtime_error, "toTruecolor expects a Color::Format parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::RGB565 || format == Color::Format::XRGB8888, std::runtime_error, "Color format must be in [RGB555, RGB565, RGB888]");
        auto result = data;
        // convert colors if needed
        if (format == Color::Format::XRGB1555)
        {
            result.image.data = data.image.data.pixels().convertData<Color::XRGB1555>();
        }
        else if (format == Color::Format::RGB565)
        {
            result.image.data = data.image.data.pixels().convertData<Color::RGB565>();
        }
        result.image.pixelFormat = result.image.data.pixels().format();
        result.image.colorMapFormat = result.image.data.colorMap().format();
        result.image.nrOfColorMapEntries = 0;
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::toUniqueTileMap(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap() && data.type.isTiles(), std::runtime_error, "toUniqueTileMap expects tiled bitmaps as input data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "toUniqueTileMap expects a bool detect flips parameter");
        const auto detectFlips = VariantHelpers::getValue<bool, 0>(parameters);
        auto result = data;
        auto screenAndTileMap = buildUniqueTileMap(data.image.data.pixels(), data.image.size.width(), data.image.size.height(), detectFlips);
        result.map.size = result.image.size;
        result.map.data = screenAndTileMap.first;
        result.image.data.pixels() = screenAndTileMap.second;
        result.type.setBitmap(false);
        return result;
    }

    Data Processing::toTiles(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap() || data.type.isSprites(), std::runtime_error, "toTiles expects bitmaps or sprites as input data");
        auto result = data;
        result.image.data.pixels() = convertToTiles(data.image.data.pixels(), data.image.size.width(), data.image.size.height());
        result.type.setTiles();
        return result;
    }

    Data Processing::toSprites(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap() || data.type.isTiles(), std::runtime_error, "toSprites expects bitmaps or tiles as input data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "toSprites expects a uint32_t sprite width parameter");
        const auto spriteWidth = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // convert image to sprites
        auto result = data;
        result.type.setSprites();
        if (data.image.size.width() != spriteWidth)
        {
            result.image.data.pixels() = convertToWidth(data.image.data.pixels(), data.image.size.width(), data.image.size.height(), spriteWidth);
            result.image.size = {spriteWidth, (data.image.size.width() * data.image.size.height()) / spriteWidth};
        }
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::addColor0(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.image.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Adding a color can only be done for 8bit paletted images");
        REQUIRE(data.image.data.colorMap().format() == Color::Format::XRGB8888, std::runtime_error, "Adding a color can only be done for RGB888 color maps");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::XRGB8888>(parameters), std::runtime_error, "addColor0 expects a RGB888 color parameter");
        const auto color0 = VariantHelpers::getValue<Color::XRGB8888, 0>(parameters);
        // check for space in color map
        REQUIRE(data.image.data.colorMap().size() <= 255, std::runtime_error, "No space in color map (image has " << data.image.data.colorMap().size() << " colors)");
        // add color at front of color map
        auto result = data;
        result.image.data.pixels().data<uint8_t>() = ImageHelpers::incValuesBy1(data.image.data.pixels().data<uint8_t>());
        result.image.data.colorMap().data<Color::XRGB8888>() = ColorHelpers::addColorAtIndex0(data.image.data.colorMap().data<Color::XRGB8888>(), color0);
        result.image.nrOfColorMapEntries = result.image.data.colorMap().size();
        return result;
    }

    Data Processing::moveColor0(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.image.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Moving a color can only be done for 8bit paletted images");
        REQUIRE(data.image.data.colorMap().format() == Color::Format::XRGB8888, std::runtime_error, "Moving a color can only be done for RGB888 color maps");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::XRGB8888>(parameters), std::runtime_error, "moveColor0 expects a RGB888 color parameter");
        const auto color0 = VariantHelpers::getValue<Color::XRGB8888, 0>(parameters);
        // try to find color in palette
        auto colorMap = data.image.data.colorMap().data<Color::XRGB8888>();
        auto oldColorIt = std::find(colorMap.begin(), colorMap.end(), color0);
        REQUIRE(oldColorIt != colorMap.end(), std::runtime_error, "Color " << color0.toHex() << " not found in image color map");
        const size_t oldIndex = std::distance(colorMap.begin(), oldColorIt);
        // check if index needs to move
        if (oldIndex != 0)
        {
            auto result = data;
            // move index in color map and pixel data
            std::swap(colorMap[oldIndex], colorMap[0]);
            result.image.data.colorMap().data<Color::XRGB8888>() = colorMap;
            result.image.data.pixels().data<uint8_t>() = ImageHelpers::swapValueWith0(data.image.data.pixels().data<uint8_t>(), oldIndex);
            return result;
        }
        return data;
    }

    Data Processing::reorderColors(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.image.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Reordering colors can only be done for 8bit paletted images");
        REQUIRE(data.image.data.colorMap().format() == Color::Format::XRGB8888, std::runtime_error, "Reordering colors can only be done for RGB888 color maps");
        const auto newOrder = ColorHelpers::optimizeColorDistance(data.image.data.colorMap().data<Color::XRGB8888>());
        auto result = data;
        result.image.data.pixels().data<uint8_t>() = ImageHelpers::swapValues(data.image.data.pixels().data<uint8_t>(), newOrder);
        result.image.data.colorMap().data<Color::XRGB8888>() = ColorHelpers::swapColors(data.image.data.colorMap().data<Color::XRGB8888>(), newOrder);
        return result;
    }

    Data Processing::shiftIndices(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.image.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Shifting indices can only be done for 8bit paletted images");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "shiftIndices expects a uint32_t shift parameter");
        const auto shiftBy = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // find max. index value
        auto &dataIndices = data.image.data.pixels().data<uint8_t>();
        auto maxIndex = *std::max_element(dataIndices.cbegin(), dataIndices.cend());
        REQUIRE(maxIndex + shiftBy <= 255, std::runtime_error, "Max. index value in image is " << maxIndex << ", shift is " << shiftBy << "! Resulting index values would be > 255");
        // shift indices
        Data result = data;
        auto &resultIndices = result.image.data.pixels().data<uint8_t>();
        std::for_each(resultIndices.begin(), resultIndices.end(), [shiftBy](auto &index)
                      { index = (index == 0) ? 0 : (((index + shiftBy) > 255) ? 255 : (index + shiftBy)); });
        return result;
    }

    Data Processing::pruneIndices(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.image.data.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Index pruning only possible for 8bit paletted images");
        REQUIRE(data.image.data.colorMap().size() <= 16, std::runtime_error, "Index pruning only possible for images with <= 16 colors");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "pruneIndices expects a uint32_t bit depth parameter");
        const auto bitDepth = VariantHelpers::getValue<uint32_t, 0>(parameters);
        REQUIRE(bitDepth == 1 || bitDepth == 2 || bitDepth == 4, std::runtime_error, "Bit depth must be in [1, 2, 4]");
        auto result = data;
        auto &indices = result.image.data.pixels().data<uint8_t>();
        auto maxIndex = *std::max_element(indices.cbegin(), indices.cend());
        if (bitDepth == 1)
        {
            REQUIRE(maxIndex == 1, std::runtime_error, "Index pruning to 1 bit only possible with index data <= 1");
            result.image.data.pixels() = PixelData(ImageHelpers::convertDataTo1Bit(indices), Color::Format::Paletted1);
        }
        else if (bitDepth == 2)
        {
            REQUIRE(maxIndex < 4, std::runtime_error, "Index pruning to 2 bit only possible with index data <= 3");
            result.image.data.pixels() = PixelData(ImageHelpers::convertDataTo2Bit(indices), Color::Format::Paletted2);
        }
        else
        {
            REQUIRE(maxIndex < 16, std::runtime_error, "Index pruning to 4 bit only possible with index data <= 15");
            result.image.data.pixels() = PixelData(ImageHelpers::convertDataTo4Bit(indices), Color::Format::Paletted4);
        }
        result.image.pixelFormat = result.image.data.pixels().format();
        return result;
    }

    Data Processing::toDelta8(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        auto result = data;
        result.image.data.pixels() = PixelData(DataHelpers::deltaEncode(result.image.data.pixels().convertDataToRaw()), Color::Format::Unknown);
        result.type.setCompressed();
        return result;
    }

    Data Processing::toDelta16(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        auto result = data;
        result.image.data.pixels() = PixelData(DataHelpers::convertTo<uint8_t>(DataHelpers::deltaEncode(DataHelpers::convertTo<uint16_t>(result.image.data.pixels().convertDataToRaw()))), Color::Format::Unknown);
        result.type.setCompressed();
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::compressLZ10(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "compressLZ10 expects a bool VRAMcompatible parameter");
        const auto vramCompatible = VariantHelpers::getValue<bool, 0>(parameters);
        // compress data
        auto result = data;
        result.image.data.pixels() = PixelData(Compression::encodeLZ10(result.image.data.pixels().convertDataToRaw(), vramCompatible), Color::Format::Unknown);
        result.type.setCompressed();
        return result;
    }

    Data Processing::compressRLE(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
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

    Data Processing::compressDXT(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "compressDXT expects bitmaps as input data");
        REQUIRE(data.image.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "DXT compression is only possible for RGB888 truecolor images");
        REQUIRE(data.image.size.width() % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
        REQUIRE(data.image.size.height() % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT compression");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::Format>(parameters), std::runtime_error, "compressDXT expects a Color::Format parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::RGB565 ||
                    format == Color::Format::XBGR1555 || format == Color::Format::BGR565,
                std::runtime_error, "Output color format must be in [RGB555, RGB565, BGR555, BGR565]");
        // convert image using DXT compression
        auto result = data;
        auto compressedData = DXT::encode(data.image.data.pixels().data<Color::XRGB8888>(), data.image.size.width(), data.image.size.height(), format == Color::Format::RGB565, format == Color::Format::XBGR1555 || format == Color::Format::BGR565);
        result.image.data.pixels() = PixelData(compressedData, Color::Format::Unknown);
        result.image.pixelFormat = format;
        result.image.colorMapFormat = Color::Format::Unknown;
        result.type.setCompressed();
        return result;
    }

    Data Processing::compressDXTV(const Data &data, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "compressDXTV expects bitmaps as input data");
        REQUIRE(data.image.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "DXTV compression is only possible for RGB888 truecolor images");
        REQUIRE(data.image.size.width() % 8 == 0, std::runtime_error, "Image width must be a multiple of 8 for DXTV compression");
        REQUIRE(data.image.size.height() % 8 == 0, std::runtime_error, "Image height must be a multiple of 8 for DXTV compression");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Color::Format, double>(parameters)), std::runtime_error, "compressDXTV expects a Color::Format and a double quality parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::XBGR1555, std::runtime_error, "Output color format must be in [RGB555, BGR555]");
        auto quality = VariantHelpers::getValue<double, 1>(parameters);
        REQUIRE(quality >= 0 && quality <= 100, std::runtime_error, "compressDXTV quality must be in [0, 100]");
        // convert image using DXTV compression
        auto result = data;
        auto previousImage = state.empty() ? std::vector<Color::XRGB8888>() : DataHelpers::convertTo<Color::XRGB8888>(state);
        auto compressedData = DXTV::encode(data.image.data.pixels().data<Color::XRGB8888>(), previousImage, data.image.size.width(), data.image.size.height(), quality, format == Color::Format::XBGR1555);
        result.image.data.pixels() = PixelData(compressedData.first, Color::Format::Unknown);
        result.image.pixelFormat = format;
        result.image.colorMapFormat = Color::Format::Unknown;
        result.type.setCompressed();
        // store decompressed image as state
        state = DataHelpers::convertTo<uint8_t>(compressedData.second);
        // add statistics
        if (statistics != nullptr)
        {
            statistics->setImage("DXTV output", state, Color::Format::XRGB8888, result.image.size.width(), result.image.size.height());
        }
        return result;
    }

    Data Processing::compressGVID(const Data &data, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.type.isBitmap(), std::runtime_error, "compressGVID expects bitmaps as input data");
        REQUIRE(data.image.data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "GVID compression is only possible for RGB888 truecolor images");
        REQUIRE(data.image.size.width() % 16 == 0, std::runtime_error, "Image width must be a multiple of 16 for GVID compression");
        REQUIRE(data.image.size.height() % 16 == 0, std::runtime_error, "Image height must be a multiple of 16 for GVID compression");
        auto result = data;
        auto compressedData = GVID::encodeGVID(data.image.data.pixels().data<Color::XRGB8888>(), data.image.size.width(), data.image.size.height());
        result.image.data.pixels() = PixelData(compressedData, Color::Format::Unknown);
        result.image.pixelFormat = Color::Format::YCgCoRf;
        result.image.colorMapFormat = Color::Format::Unknown;
        result.type.setCompressed();
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::convertPixelsToRaw(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::Format>(parameters), std::runtime_error, "convertPixelsToRaw expects a Color::Format parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::RGB565 || format == Color::Format::XRGB8888 ||
                    format == Color::Format::XBGR1555 || format == Color::Format::BGR565 || format == Color::Format::XBGR8888,
                std::runtime_error, "Color format must be in [RGB555, RGB565, RGB888, BGR555, BGR565, BGR888]");
        // if pixel data is already raw return it
        if (data.image.data.pixels().isRaw())
        {
            return data;
        }
        auto result = data;
        // if pixel data is indexed we don't need to convert the color format
        if (data.image.data.pixels().isIndexed())
        {
            result.image.data.pixels() = PixelData(result.image.data.pixels().convertDataToRaw(), Color::Format::Unknown);
        }
        else
        {
            result.image.data.pixels() = PixelData(result.image.data.pixels().convertTo(format).convertDataToRaw(), Color::Format::Unknown);
            result.image.pixelFormat = format;
        }
        return result;
    }

    Data Processing::padPixelData(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.image.data.pixels().isRaw(), std::runtime_error, "Pixel data padding is only possible for raw data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "padPixelData expects a uint32_t pad modulo parameter");
        auto multipleOf = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // pad pixel data
        auto result = data;
        result.image.data.pixels() = PixelData(DataHelpers::fillUpToMultipleOf(data.image.data.pixels().convertDataToRaw(), multipleOf), Color::Format::Unknown);
        return result;
    }

    Data Processing::convertColorMapToRaw(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::Format>(parameters), std::runtime_error, "convertColorMapToRaw expects a Color::Format parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::RGB565 || format == Color::Format::XRGB8888 ||
                    format == Color::Format::XBGR1555 || format == Color::Format::BGR565 || format == Color::Format::XBGR8888,
                std::runtime_error, "Color format must be in [RGB555, RGB565, RGB888, BGR555, BGR565, BGR888]");
        REQUIRE(!data.image.data.colorMap().empty(), std::runtime_error, "Color map can not be empty");
        // if color map data is already raw return it
        if (data.image.data.colorMap().isRaw())
        {
            return data;
        }
        auto result = data;
        result.image.data.colorMap() = PixelData(result.image.data.colorMap().convertTo(format).convertDataToRaw(), Color::Format::Unknown);
        result.image.colorMapFormat = format;
        return result;
    }

    Data Processing::padMapData(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
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

    Data Processing::padColorMap(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "padColorMap expects a uint32_t pad modulo parameter");
        auto multipleOf = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // pad data
        auto result = data;
        result.image.data.colorMap() = std::visit([multipleOf, format = data.image.data.colorMap().format()](auto colorMap) -> PixelData
                                                  { 
                using T = std::decay_t<decltype(colorMap)>;
                if constexpr (std::is_same<T, std::vector<Color::XRGB1555>>() || std::is_same<T, std::vector<Color::RGB565>>() || std::is_same<T, std::vector<Color::XRGB8888>>())
                {
                    return PixelData(DataHelpers::fillUpToMultipleOf(colorMap, multipleOf), format);
                }
                THROW(std::runtime_error, "Color format must be XRGB1555, RGB565 or XRGB8888"); },
                                                  data.image.data.colorMap().storage());
        result.image.nrOfColorMapEntries = result.image.data.colorMap().size();
        return result;
    }

    Data Processing::padColorMapData(const Data &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(data.image.data.colorMap().isRaw(), std::runtime_error, "Color map data padding is only possible for raw data");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padColorMapData expects a single uint32_t pad modulo parameter");
        auto multipleOf = std::get<uint32_t>(parameters.front());
        // pad raw color map data
        auto result = data;
        result.image.data.colorMap() = PixelData(DataHelpers::fillUpToMultipleOf(data.image.data.colorMap().convertDataToRaw(), multipleOf), Color::Format::Unknown);
        return result;
    }

    std::vector<Data> Processing::equalizeColorMaps(const std::vector<Data> &images, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics)
    {
        auto allColorMapsSameSize = std::find_if_not(images.cbegin(), images.cend(), [refSize = images.front().image.data.colorMap().size()](const auto &img)
                                                     { return img.image.data.colorMap().size() == refSize; }) == images.cend();
        // pad color map if necessary
        if (!allColorMapsSameSize)
        {
            uint32_t maxColorMapColors = std::max_element(images.cbegin(), images.cend(), [](const auto &imgA, const auto &imgB)
                                                          { return imgA.image.data.colorMap().size() < imgB.image.data.colorMap().size(); })
                                             ->image.data.colorMap()
                                             .size();
            std::vector<Data> result;
            std::transform(images.begin(), images.end(), std::back_inserter(result), [maxColorMapColors, statistics](auto &img)
                           { return padColorMap(img, {Parameter(maxColorMapColors)}, statistics); });
            return result;
        }
        return images;
    }

    Data Processing::pixelDiff(const Data &data, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Frame::SPtr statistics)
    {
        // check if a usable state was passed
        if (!state.empty())
        {
            auto result = data;
            // calculate difference and set state
            result.image.data.pixels() = std::visit([&state, format = data.image.data.pixels().format()](auto currentPixels) -> PixelData
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
                                                    data.image.data.pixels().storage());
            return result;
        }
        // set current image to state
        state = data.image.data.pixels().convertDataToRaw();
        // no state, return input data
        return data;
    }

    void Processing::addStep(ProcessingType type, const std::vector<Parameter> &parameters, bool prependProcessingInfo, bool addStatistics)
    {
        m_steps.push_back({type, parameters, prependProcessingInfo, addStatistics});
    }

    std::size_t Processing::size() const
    {
        return m_steps.size();
    }

    void Processing::clear()
    {
        m_steps.clear();
    }

    void Processing::clearState()
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
            const auto &stepFunc = ProcessingFunctions.find(step.type)->second;
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

    Data prependProcessingInfo(const Data &processedData, uint32_t originalSize, ProcessingType type, bool isFinal)
    {
        auto rawData = processedData.image.data.pixels().convertDataToRaw();
        REQUIRE(rawData.size() < (1 << 24), std::runtime_error, "Raw data size stored must be < 16MB");
        REQUIRE(static_cast<uint32_t>(type) <= 127, std::runtime_error, "Type value must be <= 127");
        const uint32_t sizeAndType = ((originalSize & 0xFFFFFF) << 8) | ((static_cast<uint32_t>(type) & 0x7F) | (isFinal ? static_cast<uint32_t>(ProcessingTypeFinal) : 0));
        auto result = processedData;
        result.image.data.pixels() = PixelData(DataHelpers::prependValue(rawData, sizeAndType), Color::Format::Unknown);
        return result;
    }

    std::vector<Data> Processing::processBatch(const std::vector<Data> &data)
    {
        REQUIRE(data.size() > 0, std::runtime_error, "Empty data passed to processing");
        bool finalStepFound = false;
        std::vector<Data> processed = data;
        for (auto stepIt = m_steps.begin(); stepIt != m_steps.end(); ++stepIt)
        {
            auto &stepFunc = ProcessingFunctions.find(stepIt->type)->second;
            // check if this was the final processing step (first non-input processing)
            bool isFinalStep = false;
            if (!finalStepFound)
            {
                isFinalStep = true;
                finalStepFound = true;
            }
            // process depending on operation type
            if (stepFunc.type == OperationType::Convert)
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc.func);
                for (auto &img : processed)
                {
                    const auto inputSize = img.image.data.pixels().rawSize();
                    img = convertFunc(img, stepIt->parameters, nullptr);
                    if (stepIt->prependProcessingInfo)
                    {
                        img = prependProcessingInfo(img, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : img.image.data.pixels().rawSize() + sizeof(uint32_t);
                    img.image.maxMemoryNeeded = (img.image.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : img.image.maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::ConvertState)
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc.func);
                for (auto &img : processed)
                {
                    const uint32_t inputSize = img.image.data.pixels().rawSize();
                    img = convertFunc(img, stepIt->parameters, stepIt->state, nullptr);
                    if (stepIt->prependProcessingInfo)
                    {
                        img = prependProcessingInfo(img, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : img.image.data.pixels().rawSize() + sizeof(uint32_t);
                    img.image.maxMemoryNeeded = (img.image.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : img.image.maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::BatchConvert)
            {
                // get all input sizes
                std::vector<uint32_t> inputSizes = {};
                std::transform(processed.cbegin(), processed.cend(), std::back_inserter(inputSizes), [](const auto &d)
                               { return d.image.data.pixels().rawSize(); });
                auto batchFunc = std::get<BatchConvertFunc>(stepFunc.func);
                processed = batchFunc(processed, stepIt->parameters, nullptr);
                for (auto pIt = processed.begin(); pIt != processed.end(); pIt++)
                {
                    if (stepIt->prependProcessingInfo)
                    {
                        const uint32_t inputSize = inputSizes.at(std::distance(processed.begin(), pIt));
                        *pIt = prependProcessingInfo(*pIt, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : pIt->image.data.pixels().rawSize() + sizeof(uint32_t);
                    pIt->image.maxMemoryNeeded = (pIt->image.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : pIt->image.maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::Reduce)
            {
                auto reduceFunc = std::get<ReduceFunc>(stepFunc.func);
                processed = {reduceFunc(processed, stepIt->parameters, nullptr)};
            }
        }
        return processed;
    }

    Data Processing::processStream(const Data &data, Statistics::Container::SPtr statistics)
    {
        bool finalStepFound = false;
        Data processed = data;
        auto frameStatistics = statistics != nullptr ? statistics->addFrame() : nullptr;
        for (auto stepIt = m_steps.begin(); stepIt != m_steps.end(); ++stepIt)
        {
            const uint32_t inputSize = processed.image.data.pixels().rawSize();
            auto stepStatistics = stepIt->addStatistics ? frameStatistics : nullptr;
            auto &stepFunc = ProcessingFunctions.find(stepIt->type)->second;
            if (stepFunc.type == OperationType::Convert)
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc.func);
                processed = convertFunc(processed, stepIt->parameters, stepStatistics);
            }
            else if (stepFunc.type == OperationType::ConvertState)
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc.func);
                processed = convertFunc(processed, stepIt->parameters, stepIt->state, stepStatistics);
            }
            // we're silently ignoring OperationType::BatchInput ::BatchConvert and ::Reduce operations here
            if (stepIt->prependProcessingInfo)
            {
                // check if this was the final processing step (first non-input processing)
                bool isFinalStep = false;
                if (!finalStepFound)
                {
                    isFinalStep = true;
                    finalStepFound = true;
                }
                processed = prependProcessingInfo(processed, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
            }
            // record max. memory needed for everything, but the first step
            auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : processed.image.data.pixels().rawSize() + sizeof(uint32_t);
            processed.image.maxMemoryNeeded = (processed.image.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : processed.image.maxMemoryNeeded;
        }
        return processed;
    }
}

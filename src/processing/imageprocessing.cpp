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
            {ProcessingType::CompressLZ11, {"compress LZ11", OperationType::Convert, FunctionType(compressLZ11)}},
            //{ProcessingType::CompressRLE, {"compress RLE", OperationType::Convert, FunctionType(compressRLE)}},
            {ProcessingType::CompressDXT, {"compress DXT", OperationType::Convert, FunctionType(compressDXT)}},
            {ProcessingType::CompressDXTV, {"compress DXTV", OperationType::ConvertState, FunctionType(compressDXTV)}},
            {ProcessingType::CompressGVID, {"compress GVID", OperationType::ConvertState, FunctionType(compressGVID)}},
            {ProcessingType::PadPixelData, {"pad pixel data", OperationType::Convert, FunctionType(padPixelData)}},
            {ProcessingType::PadColorMap, {"pad color map", OperationType::Convert, FunctionType(padColorMap)}},
            {ProcessingType::ConvertColorMap, {"convert color map", OperationType::Convert, FunctionType(convertColorMap)}},
            {ProcessingType::PadColorMapData, {"pad color map data", OperationType::Convert, FunctionType(padColorMapData)}},
            {ProcessingType::EqualizeColorMaps, {"equalize color maps", OperationType::BatchConvert, FunctionType(equalizeColorMaps)}},
            {ProcessingType::DeltaImage, {"pixel diff", OperationType::ConvertState, FunctionType(pixelDiff)}}};

    Data Processing::toBlackWhite(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.dataType == DataType::Bitmap, std::runtime_error, "toBlackWhite expects bitmaps as input data");
        REQUIRE(data.imageData.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "Expected RGB888 input data");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Quantization::Method, double>(parameters)), std::runtime_error, "toBlackWhite expects a Quantization::Method and double threshold parameter");
        const auto quantizationMethod = VariantHelpers::getValue<Quantization::Method, 0>(parameters);
        const auto threshold = VariantHelpers::getValue<double, 1>(parameters);
        REQUIRE(threshold >= 0 && threshold <= 1, std::runtime_error, "Threshold must be in [0.0, 1.0]");
        // threshold image
        auto result = data;
        result.imageData = Quantization::quantizeThreshold(data.imageData, static_cast<float>(threshold));
        REQUIRE(result.imageData.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted image");
        return result;
    }

    Data Processing::toPaletted(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.dataType == DataType::Bitmap, std::runtime_error, "toPaletted expects bitmaps as input data");
        REQUIRE(data.imageData.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "toPaletted expects RGB888 input data");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Quantization::Method, uint32_t, std::vector<Color::XRGB8888>>(parameters)), std::runtime_error, "toPaletted expects a Quantization::Method, uint32_t number of colors parameter and a std::vector<Color::XRGB8888> color-space map");
        const auto quantizationMethod = VariantHelpers::getValue<Quantization::Method, 0>(parameters);
        const auto nrOfColors = VariantHelpers::getValue<uint32_t, 1>(parameters);
        REQUIRE(nrOfColors >= 2 && nrOfColors <= 256, std::runtime_error, "Number of colors must be in [2, 256]");
        const auto colorSpaceMap = VariantHelpers::getValue<std::vector<Color::XRGB8888>, 2>(parameters);
        REQUIRE(colorSpaceMap.size() > 0, std::runtime_error, "colorSpaceMap can not be empty");
        // convert image to paletted using dithering
        auto result = data;
        switch (quantizationMethod)
        {
        case Quantization::Method::ClosestColor:
            result.imageData = Quantization::quantizeClosest(data.imageData, nrOfColors, colorSpaceMap);
            break;
        case Quantization::Method::AtkinsonDither:
            result.imageData = Quantization::atkinsonDither(data.imageData, nrOfColors, colorSpaceMap);
            break;
        default:
            THROW(std::runtime_error, "Unsupported quantization method " << Quantization::toString(quantizationMethod));
        }
        REQUIRE(result.imageData.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted return image");
        return result;
    }

    std::vector<Data> Processing::toCommonPalette(const std::vector<Data> &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.size() > 1, std::runtime_error, "toCommonPalette expects more than one input image");
        REQUIRE(data.front().dataType == DataType::Bitmap, std::runtime_error, "toCommonPalette expects bitmaps as input data");
        REQUIRE(data.front().imageData.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "toCommonPalette expects RGB888 input data");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<Quantization::Method, uint32_t, std::vector<Color::XRGB8888>>(parameters)), std::runtime_error, "toCommonPalette expects a Quantization::Method, uint32_t number of colors parameter and a std::vector<Color::XRGB8888> color-space map");
        const auto quantizationMethod = VariantHelpers::getValue<Quantization::Method, 0>(parameters);
        const auto nrOfColors = VariantHelpers::getValue<uint32_t, 1>(parameters);
        REQUIRE(nrOfColors >= 2 && nrOfColors <= 256, std::runtime_error, "Number of colors must be in [2, 256]");
        const auto colorSpaceMap = VariantHelpers::getValue<std::vector<Color::XRGB8888>, 2>(parameters);
        REQUIRE(colorSpaceMap.size() > 0, std::runtime_error, "colorSpaceMap can not be empty");
        // set up number of cores for parallel processing
        const auto nrOfProcessors = omp_get_num_procs();
        omp_set_num_threads(nrOfProcessors);
        // build histogram of colors used in all input images
        std::cout << "Building histogram..." << std::endl;
        std::map<Color::XRGB8888, uint64_t> histogram;
        std::for_each(data.cbegin(), data.cend(), [&histogram](const auto &d)
                      {
            auto & imageData = d.imageData.pixels().template data<Color::XRGB8888>();
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
                               r.imageData = Quantization::quantizeClosest(d.imageData, nrOfColors, commonColorMap);
                               break;
                           case Quantization::Method::AtkinsonDither:
                               r.imageData = Quantization::atkinsonDither(d.imageData, nrOfColors, commonColorMap);
                               break;
                           default:
                               THROW(std::runtime_error, "Unsupported quantization method " << Quantization::toString(quantizationMethod));
                           }
                           REQUIRE(r.imageData.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted return image");
                           // auto dumpPath = std::filesystem::current_path() / "result" / (std::to_string(d.index) + ".png");
                           // temp.write(dumpPath.c_str());
                           return r;
                       });
        return result;
    }

    Data Processing::toTruecolor(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.dataType == DataType::Bitmap, std::runtime_error, "toTruecolor expects bitmaps as input data");
        REQUIRE(data.imageData.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "toTruecolor expects a RGB888 image");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::Format>(parameters), std::runtime_error, "toTruecolor expects a Color::Format parameter");
        const auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::RGB565 || format == Color::Format::XRGB8888, std::runtime_error, "Color format must be in [RGB555, RGB565, RGB888]");
        auto result = data;
        // convert colors if needed
        if (format == Color::Format::XRGB1555)
        {
            result.imageData = data.imageData.pixels().convertData<Color::XRGB1555>();
        }
        else if (format == Color::Format::RGB565)
        {
            result.imageData = data.imageData.pixels().convertData<Color::RGB565>();
        }
        return result;
    }

    Data Processing::toRaw(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        auto result = data;
        result.imageData.pixels() = PixelData(result.imageData.pixels().convertDataToRaw(), Color::Format::Unknown);
        result.imageData.colorMap() = PixelData(result.imageData.colorMap().convertDataToRaw(), Color::Format::Unknown);
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::toUniqueTileMap(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.dataType == DataType::Tilemap, std::runtime_error, "toUniqueTileMap expects tiles as input data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "toUniqueTileMap expects a bool detect flips parameter");
        const auto detectFlips = VariantHelpers::getValue<bool, 0>(parameters);
        auto result = data;
        auto screenAndTileMap = buildUniqueTileMap(data.imageData.pixels(), data.size.width(), data.size.height(), detectFlips);
        result.mapData = screenAndTileMap.first;
        result.imageData.pixels() = screenAndTileMap.second;
        result.dataType = DataType::Tilemap;
        return result;
    }

    Data Processing::toTiles(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.dataType == DataType::Bitmap, std::runtime_error, "toTiles expects bitmaps as input data");
        auto result = data;
        result.imageData.pixels() = convertToTiles(data.imageData.pixels(), data.size.width(), data.size.height());
        return result;
    }

    Data Processing::toSprites(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.dataType == DataType::Bitmap, std::runtime_error, "toSprites expects bitmaps as input data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "toSprites expects a uint32_t sprite width parameter");
        const auto spriteWidth = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // convert image to sprites
        if (data.size.width() != spriteWidth)
        {
            auto result = data;
            result.imageData.pixels() = convertToWidth(data.imageData.pixels(), data.size.width(), data.size.height(), spriteWidth);
            result.size = {spriteWidth, (data.size.width() * data.size.height()) / spriteWidth};
            return result;
        }
        return data;
    }

    Data Processing::addColor0(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.imageData.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Adding a color can only be done for 8bit paletted images");
        REQUIRE(data.imageData.colorMap().format() == Color::Format::XRGB8888, std::runtime_error, "Adding a color can only be done for RGB888 color maps");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::XRGB8888>(parameters), std::runtime_error, "addColor0 expects a RGB888 color parameter");
        const auto color0 = VariantHelpers::getValue<Color::XRGB8888, 0>(parameters);
        // check for space in color map
        REQUIRE(data.imageData.colorMap().size() <= 255, std::runtime_error, "No space in color map (image has " << data.imageData.colorMap().size() << " colors)");
        // add color at front of color map
        auto result = data;
        result.imageData.pixels().data<uint8_t>() = ImageHelpers::incValuesBy1(data.imageData.pixels().data<uint8_t>());
        result.imageData.colorMap().data<Color::XRGB8888>() = ColorHelpers::addColorAtIndex0(data.imageData.colorMap().data<Color::XRGB8888>(), color0);
        return result;
    }

    Data Processing::moveColor0(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.imageData.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Moving a color can only be done for 8bit paletted images");
        REQUIRE(data.imageData.colorMap().format() == Color::Format::XRGB8888, std::runtime_error, "Moving a color can only be done for RGB888 color maps");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::XRGB8888>(parameters), std::runtime_error, "moveColor0 expects a RGB888 color parameter");
        const auto color0 = VariantHelpers::getValue<Color::XRGB8888, 0>(parameters);
        // try to find color in palette
        auto colorMap = data.imageData.colorMap().data<Color::XRGB8888>();
        auto oldColorIt = std::find(colorMap.begin(), colorMap.end(), color0);
        REQUIRE(oldColorIt != colorMap.end(), std::runtime_error, "Color " << color0.toHex() << " not found in image color map");
        const size_t oldIndex = std::distance(colorMap.begin(), oldColorIt);
        // check if index needs to move
        if (oldIndex != 0)
        {
            auto result = data;
            // move index in color map and pixel data
            std::swap(colorMap[oldIndex], colorMap[0]);
            result.imageData.colorMap().data<Color::XRGB8888>() = colorMap;
            result.imageData.pixels().data<uint8_t>() = ImageHelpers::swapValueWith0(data.imageData.pixels().data<uint8_t>(), oldIndex);
            return result;
        }
        return data;
    }

    Data Processing::reorderColors(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.imageData.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Reordering colors can only be done for 8bit paletted images");
        REQUIRE(data.imageData.colorMap().format() == Color::Format::XRGB8888, std::runtime_error, "Reordering colors can only be done for RGB888 color maps");
        const auto newOrder = ColorHelpers::optimizeColorDistance(data.imageData.colorMap().data<Color::XRGB8888>());
        auto result = data;
        result.imageData.pixels().data<uint8_t>() = ImageHelpers::swapValues(data.imageData.pixels().data<uint8_t>(), newOrder);
        result.imageData.colorMap().data<Color::XRGB8888>() = ColorHelpers::swapColors(data.imageData.colorMap().data<Color::XRGB8888>(), newOrder);
        return result;
    }

    Data Processing::shiftIndices(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.imageData.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Shifting indices can only be done for 8bit paletted images");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "shiftIndices expects a uint32_t shift parameter");
        const auto shiftBy = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // find max. index value
        auto &dataIndices = data.imageData.pixels().data<uint8_t>();
        auto maxIndex = *std::max_element(dataIndices.cbegin(), dataIndices.cend());
        REQUIRE(maxIndex + shiftBy <= 255, std::runtime_error, "Max. index value in image is " << maxIndex << ", shift is " << shiftBy << "! Resulting index values would be > 255");
        // shift indices
        Data result = data;
        auto &resultIndices = result.imageData.pixels().data<uint8_t>();
        std::for_each(resultIndices.begin(), resultIndices.end(), [shiftBy](auto &index)
                      { index = (index == 0) ? 0 : (((index + shiftBy) > 255) ? 255 : (index + shiftBy)); });
        return result;
    }

    Data Processing::pruneIndices(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.imageData.pixels().format() == Color::Format::Paletted8, std::runtime_error, "Index pruning only possible for 8bit paletted images");
        REQUIRE(data.imageData.colorMap().size() <= 16, std::runtime_error, "Index pruning only possible for images with <= 16 colors");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "pruneIndices expects a uint32_t bit depth parameter");
        const auto bitDepth = VariantHelpers::getValue<uint32_t, 0>(parameters);
        REQUIRE(bitDepth == 1 || bitDepth == 2 || bitDepth == 4, std::runtime_error, "Bit depth must be in [1, 2, 4]");
        auto result = data;
        auto &indices = result.imageData.pixels().data<uint8_t>();
        auto maxIndex = *std::max_element(indices.cbegin(), indices.cend());
        if (bitDepth == 1)
        {
            REQUIRE(maxIndex == 1, std::runtime_error, "Index pruning to 1 bit only possible with index data <= 1");
            result.imageData.pixels() = PixelData(ImageHelpers::convertDataTo1Bit(indices), Color::Format::Paletted1);
        }
        else if (bitDepth == 2)
        {
            REQUIRE(maxIndex < 4, std::runtime_error, "Index pruning to 2 bit only possible with index data <= 3");
            result.imageData.pixels() = PixelData(ImageHelpers::convertDataTo2Bit(indices), Color::Format::Paletted2);
        }
        else
        {
            REQUIRE(maxIndex < 16, std::runtime_error, "Index pruning to 4 bit only possible with index data <= 15");
            result.imageData.pixels() = PixelData(ImageHelpers::convertDataTo4Bit(indices), Color::Format::Paletted4);
        }
        return result;
    }

    Data Processing::toDelta8(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        auto result = data;
        result.imageData.pixels() = PixelData(DataHelpers::deltaEncode(result.imageData.pixels().convertDataToRaw()), Color::Format::Unknown);
        return result;
    }

    Data Processing::toDelta16(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        auto result = data;
        result.imageData.pixels() = PixelData(DataHelpers::convertTo<uint8_t>(DataHelpers::deltaEncode(DataHelpers::convertTo<uint16_t>(result.imageData.pixels().convertDataToRaw()))), Color::Format::Unknown);
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::compressLZ10(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "compressLZ10 expects a bool VRAMcompatible parameter");
        const auto vramCompatible = VariantHelpers::getValue<bool, 0>(parameters);
        // compress data
        auto result = data;
        result.imageData.pixels() = PixelData(Compression::compressLzss(result.imageData.pixels().convertDataToRaw(), vramCompatible, false), Color::Format::Unknown);
        // result.data = LZSS::encodeLZSS(image.data, vramCompatible);
        return result;
    }

    Data Processing::compressLZ11(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "compressLZ11 expects a bool VRAMcompatible parameter");
        const auto vramCompatible = VariantHelpers::getValue<bool, 0>(parameters);
        // compress data
        auto result = data;
        result.imageData.pixels() = PixelData(Compression::compressLzss(result.imageData.pixels().convertDataToRaw(), vramCompatible, true), Color::Format::Unknown);
        return result;
    }

    Data Processing::compressRLE(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "compressRLE expects a bool VRAMcompatible parameter");
        const auto vramCompatible = VariantHelpers::getValue<bool, 0>(parameters);
        // compress data
        auto result = data;
        // result.data = RLE::encodeRLE(image.data, vramCompatible);
        return result;
    }

    Data Processing::compressDXT(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.dataType == DataType::Bitmap, std::runtime_error, "compressDXT expects bitmaps as input data");
        REQUIRE(data.imageData.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "DXT compression is only possible for RGB888 truecolor images");
        REQUIRE(data.size.width() % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
        REQUIRE(data.size.height() % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT compression");
        // convert image using DXT compression
        auto result = data;
        auto compressedData = DXT::encodeDXT(data.imageData.pixels().data<Color::XRGB8888>(), data.size.width(), data.size.height());
        result.imageData.pixels() = PixelData(compressedData, Color::Format::Unknown);
        return result;
    }

    Data Processing::compressDXTV(const Data &data, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.dataType == DataType::Bitmap, std::runtime_error, "compressDXTV expects bitmaps as input data");
        REQUIRE(data.imageData.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "DXTV compression is only possible for RGB888 truecolor images");
        REQUIRE(data.size.width() % 16 == 0, std::runtime_error, "Image width must be a multiple of 16 for DXTV compression");
        REQUIRE(data.size.height() % 16 == 0, std::runtime_error, "Image height must be a multiple of 16 for DXTV compression");
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<double, double>(parameters)), std::runtime_error, "compressDXTV expects a double keyframe interval and a double max. block error parameter");
        auto keyFrameInterval = static_cast<int32_t>(VariantHelpers::getValue<double, 0>(parameters));
        REQUIRE(keyFrameInterval >= 0 && keyFrameInterval <= 60, std::runtime_error, "compressDXTV keyframe interval must be in [0, 60] (0 = none)");
        auto maxBlockError = VariantHelpers::getValue<double, 1>(parameters);
        REQUIRE(maxBlockError >= 0.01 && maxBlockError <= 1, std::runtime_error, "compressDXTV max. block error must be in [0.01, 1]");
        // check if needs to be a keyframe
        const bool isKeyFrame = keyFrameInterval > 0 ? ((data.index % keyFrameInterval) == 0 || state.empty()) : false;
        // convert image using DXT compression
        auto result = data;
        auto previousImage = state.empty() ? std::vector<Color::XRGB8888>() : DataHelpers::convertTo<Color::XRGB8888>(state);
        auto compressedData = DXTV::encodeDXTV(data.imageData.pixels().data<Color::XRGB8888>(), previousImage, data.size.width(), data.size.height(), isKeyFrame, maxBlockError);
        result.imageData.pixels() = PixelData(compressedData.first, Color::Format::Unknown);
        // store decompressed image as state
        state = DataHelpers::convertTo<uint8_t>(compressedData.second);
        // add statistics
        if (statistics != nullptr)
        {
            statistics->addImage("DXTV output", state, Color::Format::XRGB8888, result.size.width(), result.size.height());
        }
        return result;
    }

    Data Processing::compressGVID(const Data &data, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.dataType == DataType::Bitmap, std::runtime_error, "compressGVID expects bitmaps as input data");
        REQUIRE(data.imageData.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "GVID compression is only possible for RGB888 truecolor images");
        REQUIRE(data.size.width() % 16 == 0, std::runtime_error, "Image width must be a multiple of 16 for GVID compression");
        REQUIRE(data.size.height() % 16 == 0, std::runtime_error, "Image height must be a multiple of 16 for GVID compression");
        auto result = data;
        auto compressedData = GVID::encodeGVID(data.imageData.pixels().data<Color::XRGB8888>(), data.size.width(), data.size.height());
        result.imageData.pixels() = PixelData(compressedData, Color::Format::Unknown);
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::padPixelData(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.imageData.pixels().isRaw(), std::runtime_error, "Pixel data padding is only possible for raw data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "padPixelData expects a uint32_t pad modulo parameter");
        auto multipleOf = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // pad pixel data
        auto result = data;
        result.imageData.pixels() = PixelData(DataHelpers::fillUpToMultipleOf(data.imageData.pixels().convertDataToRaw(), multipleOf), Color::Format::Unknown);
        return result;
    }

    Data Processing::padMapData(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(!data.mapData.empty(), std::runtime_error, "Map data can not be empty");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "padMapData expects a uint32_t pad modulo parameter");
        auto multipleOf = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // pad map data
        auto result = data;
        result.mapData = DataHelpers::fillUpToMultipleOf(data.mapData, multipleOf);
        return result;
    }

    Data Processing::convertColorMap(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<Color::Format>(parameters), std::runtime_error, "convertColorMap expects a Color::Format parameter");
        auto format = VariantHelpers::getValue<Color::Format, 0>(parameters);
        REQUIRE(format == Color::Format::XRGB1555 || format == Color::Format::RGB565 || format == Color::Format::XRGB8888, std::runtime_error, "convertColorMap can only convert to XRGB1555, RGB565 and XRGB8888");
        // check if we need to convert
        if (data.imageData.colorMap().format() == format)
        {
            return data;
        }
        // convert colormap
        auto result = data;
        switch (format)
        {
        case Color::Format::XRGB1555:
            result.imageData.colorMap() = PixelData(data.imageData.colorMap().convertData<Color::XRGB1555>(), Color::Format::XRGB1555);
            break;
        case Color::Format::RGB565:
            result.imageData.colorMap() = PixelData(data.imageData.colorMap().convertData<Color::RGB565>(), Color::Format::RGB565);
            break;
        case Color::Format::XRGB8888:
            result.imageData.colorMap() = PixelData(data.imageData.colorMap().convertData<Color::XRGB8888>(), Color::Format::XRGB8888);
            break;
        default:
            THROW(std::runtime_error, "Bad target color map format");
        }
        return result;
    }

    Data Processing::padColorMap(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "padColorMap expects a uint32_t pad modulo parameter");
        auto multipleOf = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // pad data
        auto result = data;
        result.imageData.colorMap() = std::visit([multipleOf, format = data.imageData.colorMap().format()](auto colorMap) -> PixelData
                                                 { 
                using T = std::decay_t<decltype(colorMap)>;
                if constexpr (std::is_same<T, std::vector<Color::XRGB1555>>() || std::is_same<T, std::vector<Color::RGB565>>() || std::is_same<T, std::vector<Color::XRGB8888>>())
                {
                    return PixelData(DataHelpers::fillUpToMultipleOf(colorMap, multipleOf), format);
                }
                THROW(std::runtime_error, "Color format must be XRGB1555, RGB565 or XRGB8888"); },
                                                 data.imageData.colorMap().storage());
        return result;
    }

    Data Processing::padColorMapData(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(data.imageData.pixels().isRaw(), std::runtime_error, "Color map data padding is only possible for raw data");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padColorMapData expects a single uint32_t pad modulo parameter");
        auto multipleOf = std::get<uint32_t>(parameters.front());
        // pad raw color map data
        auto result = data;
        result.imageData.colorMap() = PixelData(DataHelpers::fillUpToMultipleOf(data.imageData.colorMap().convertDataToRaw(), multipleOf), Color::Format::Unknown);
        return result;
    }

    std::vector<Data> Processing::equalizeColorMaps(const std::vector<Data> &images, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        auto allColorMapsSameSize = std::find_if_not(images.cbegin(), images.cend(), [refSize = images.front().imageData.colorMap().size()](const auto &img)
                                                     { return img.imageData.colorMap().size() == refSize; }) == images.cend();
        // pad color map if necessary
        if (!allColorMapsSameSize)
        {
            uint32_t maxColorMapColors = std::max_element(images.cbegin(), images.cend(), [](const auto &imgA, const auto &imgB)
                                                          { return imgA.imageData.colorMap().size() < imgB.imageData.colorMap().size(); })
                                             ->imageData.colorMap()
                                             .size();
            std::vector<Data> result;
            std::transform(images.begin(), images.end(), std::back_inserter(result), [maxColorMapColors, statistics](auto &img)
                           { return padColorMap(img, {Parameter(maxColorMapColors)}, statistics); });
            return result;
        }
        return images;
    }

    Data Processing::pixelDiff(const Data &data, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Container::SPtr statistics)
    {
        // check if a usable state was passed
        if (!state.empty())
        {
            auto result = data;
            // calculate difference and set state
            result.imageData.pixels() = std::visit([&state, format = data.imageData.pixels().format()](auto currentPixels) -> PixelData
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
                                                   data.imageData.pixels().storage());
            return result;
        }
        // set current image to state
        state = data.imageData.pixels().convertDataToRaw();
        // no state, return input data
        return data;
    }

    void Processing::setStatisticsContainer(Statistics::Container::SPtr c)
    {
        m_statistics = c;
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
        auto rawData = processedData.imageData.pixels().convertDataToRaw();
        REQUIRE(rawData.size() < (1 << 24), std::runtime_error, "Raw data size stored must be < 16MB");
        REQUIRE(static_cast<uint32_t>(type) <= 127, std::runtime_error, "Type value must be <= 127");
        const uint32_t sizeAndType = ((originalSize & 0xFFFFFF) << 8) | ((static_cast<uint32_t>(type) & 0x7F) | (isFinal ? static_cast<uint32_t>(ProcessingTypeFinal) : 0));
        auto result = processedData;
        result.imageData.pixels() = PixelData(DataHelpers::prependValue(rawData, sizeAndType), Color::Format::Unknown);
        return result;
    }

    std::vector<Data> Processing::processBatch(const std::vector<Data> &data)
    {
        REQUIRE(data.size() > 0, std::runtime_error, "Empty data passed to processing");
        bool finalStepFound = false;
        std::vector<Data> processed = data;
        for (auto stepIt = m_steps.begin(); stepIt != m_steps.end(); ++stepIt)
        {
            auto stepStatistics = stepIt->addStatistics ? m_statistics : nullptr;
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
                    const auto inputSize = img.imageData.pixels().rawSize();
                    img = convertFunc(img, stepIt->parameters, stepStatistics);
                    if (stepIt->prependProcessingInfo)
                    {
                        img = prependProcessingInfo(img, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : img.imageData.pixels().rawSize() + sizeof(uint32_t);
                    img.maxMemoryNeeded = (img.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : img.maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::ConvertState)
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc.func);
                for (auto &img : processed)
                {
                    const uint32_t inputSize = img.imageData.pixels().rawSize();
                    img = convertFunc(img, stepIt->parameters, stepIt->state, stepStatistics);
                    if (stepIt->prependProcessingInfo)
                    {
                        img = prependProcessingInfo(img, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : img.imageData.pixels().rawSize() + sizeof(uint32_t);
                    img.maxMemoryNeeded = (img.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : img.maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::BatchConvert)
            {
                // get all input sizes
                std::vector<uint32_t> inputSizes = {};
                std::transform(processed.cbegin(), processed.cend(), std::back_inserter(inputSizes), [](const auto &d)
                               { return d.imageData.pixels().rawSize(); });
                auto batchFunc = std::get<BatchConvertFunc>(stepFunc.func);
                processed = batchFunc(processed, stepIt->parameters, stepStatistics);
                for (auto pIt = processed.begin(); pIt != processed.end(); pIt++)
                {
                    if (stepIt->prependProcessingInfo)
                    {
                        const uint32_t inputSize = inputSizes.at(std::distance(processed.begin(), pIt));
                        *pIt = prependProcessingInfo(*pIt, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : pIt->imageData.pixels().rawSize() + sizeof(uint32_t);
                    pIt->maxMemoryNeeded = (pIt->maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : pIt->maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::Reduce)
            {
                auto reduceFunc = std::get<ReduceFunc>(stepFunc.func);
                processed = {reduceFunc(processed, stepIt->parameters, stepStatistics)};
            }
        }
        return processed;
    }

    Data Processing::processStream(const Data &data)
    {
        bool finalStepFound = false;
        Data processed = data;
        for (auto stepIt = m_steps.begin(); stepIt != m_steps.end(); ++stepIt)
        {
            const uint32_t inputSize = processed.imageData.pixels().rawSize();
            auto stepStatistics = stepIt->addStatistics ? m_statistics : nullptr;
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
            auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : processed.imageData.pixels().rawSize() + sizeof(uint32_t);
            processed.maxMemoryNeeded = (processed.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : processed.maxMemoryNeeded;
        }
        return processed;
    }
}
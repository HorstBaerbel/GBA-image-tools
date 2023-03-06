#include "imageprocessing.h"

#include "codec/dxt.h"
#include "codec/dxtv.h"
#include "codec/gvid.h"
#include "color/xrgb888.h"
#include "color/colorhelpers.h"
#include "compression/lzss.h"
#include "math/colorfit.h"
#include "datahelpers.h"
#include "exception.h"
#include "imagehelpers.h"
#include "spritehelpers.h"

#include <iostream>
#include <memory>
// #include <filesystem>

namespace Image
{

    const std::map<ProcessingType, Processing::ProcessingFunc>
        Processing::ProcessingFunctions = {
            {ProcessingType::InputBlackWhite, {"binary", OperationType::Input, FunctionType(toBlackWhite)}},
            {ProcessingType::InputPaletted, {"paletted", OperationType::Input, FunctionType(toPaletted)}},
            {ProcessingType::InputCommonPalette, {"common palette", OperationType::BatchInput, FunctionType(toCommonPalette)}},
            {ProcessingType::InputTruecolor, {"truecolor", OperationType::Input, FunctionType(toTruecolor)}},
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
            {ProcessingType::CompressLz10, {"compress LZ10", OperationType::Convert, FunctionType(compressLZ10)}},
            {ProcessingType::CompressLz11, {"compress LZ11", OperationType::Convert, FunctionType(compressLZ11)}},
            //{ProcessingType::CompressRLE, {"compress RLE", OperationType::Convert, FunctionType(compressRLE)}},
            {ProcessingType::CompressDXTG, {"compress DXTG", OperationType::Convert, FunctionType(compressDXTG)}},
            {ProcessingType::CompressDXTV, {"compress DXTV", OperationType::ConvertState, FunctionType(compressDXTV)}},
            {ProcessingType::CompressGVID, {"compress GVID", OperationType::ConvertState, FunctionType(compressGVID)}},
            {ProcessingType::PadImageData, {"pad image data", OperationType::Convert, FunctionType(padImageData)}},
            {ProcessingType::PadColorMap, {"pad color map", OperationType::Convert, FunctionType(padColorMap)}},
            {ProcessingType::ConvertColorMap, {"convert color map", OperationType::Convert, FunctionType(convertColorMap)}},
            {ProcessingType::PadColorMapData, {"pad color map data", OperationType::Convert, FunctionType(padColorMapData)}},
            {ProcessingType::EqualizeColorMaps, {"equalize color maps", OperationType::BatchConvert, FunctionType(equalizeColorMaps)}},
            {ProcessingType::DeltaImage, {"image diff", OperationType::ConvertState, FunctionType(imageDiff)}}};

    Data Processing::toBlackWhite(const InputData &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<double>(parameters.front()), std::runtime_error, "toBlackWhite expects a single double threshold parameter");
        const auto threshold = std::get<double>(parameters.front());
        REQUIRE(threshold >= 0 && threshold <= 1, std::runtime_error, "Threshold must be in [0.0, 1.0]");
        // threshold image
        Magick::Image temp = data.image;
        temp.threshold(Magick::Color::scaleDoubleToQuantum(threshold));
        temp.quantizeDither(false);
        temp.quantizeColors(2);
        temp.type(Magick::ImageType::PaletteType);
        // get image data and color map
        auto imageData = getImageData(temp);
        REQUIRE(imageData.second == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted image");
        return {data.index, data.fileName, temp.type(), {temp.size().width(), temp.size().height()}, DataType::Bitmap, imageData.second, {}, imageData.first, getColorMap(temp), Color::Format::Unknown, {}};
    }

    Data Processing::toPaletted(const InputData &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 2 && std::holds_alternative<Magick::Image>(parameters.front()) && std::holds_alternative<uint32_t>(parameters.back()), std::runtime_error, "toPaletted expects a Magick::Image colorSpaceMap and uint32_t nrOfColors parameter");
        const auto nrOfcolors = std::get<uint32_t>(parameters.back());
        REQUIRE(nrOfcolors >= 2 && nrOfcolors <= 256, std::runtime_error, "Number of colors must be in [2, 256]");
        const auto colorSpaceMap = std::get<Magick::Image>(parameters.front());
        // map image to input color map, no dithering
        Magick::Image temp = data.image;
        temp.map(colorSpaceMap, false);
        // convert image to paletted using dithering
        temp.quantizeDither(true);
        temp.quantizeDitherMethod(Magick::DitherMethod::RiemersmaDitherMethod);
        temp.quantizeColors(nrOfcolors);
        temp.type(Magick::ImageType::PaletteType);
        // get image data and color map
        auto imageData = getImageData(temp);
        REQUIRE(imageData.second == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted image");
        return {data.index, data.fileName, temp.type(), {temp.size().width(), temp.size().height()}, DataType::Bitmap, imageData.second, {}, imageData.first, getColorMap(temp), Color::Format::Unknown, {}};
    }

    std::vector<Data> Processing::toCommonPalette(const std::vector<InputData> &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 2 && std::holds_alternative<Magick::Image>(parameters.front()) && std::holds_alternative<uint32_t>(parameters.back()), std::runtime_error, "toCommonPalette expects a Magick::Image colorSpaceMap and uint32_t nrOfColors parameter");
        const auto nrOfColors = std::get<uint32_t>(parameters.back());
        REQUIRE(nrOfColors >= 2 && nrOfColors <= 256, std::runtime_error, "Number of colors must be in [2, 256]");
        const auto colorSpaceMap = std::get<Magick::Image>(parameters.front());
        REQUIRE(colorSpaceMap.size().width() > 0 && colorSpaceMap.size().height() > 0, std::runtime_error, "colorSpaceMap can not be empty");
        REQUIRE(data.size() > 1, std::runtime_error, "Number of input images must be > 1");
        // set up number of cores for parallel processing
        const auto nrOfProcessors = omp_get_num_procs();
        omp_set_num_threads(nrOfProcessors);
        // build histogram of colors used in all input images
        std::cout << "Building histogram..." << std::endl;
        std::map<Color::XRGB888, uint64_t> histogram;
        std::for_each(data.cbegin(), data.cend(), [&histogram](const auto &d)
                      {
            auto imageData = getImageDataXRGB888(d.image).first;
            std::for_each(imageData.cbegin(), imageData.cend(), [&histogram](auto pixel)
                { histogram[pixel]++; }); });
        std::cout << histogram.size() << " unique colors in " << data.size() << " images" << std::endl;
        // create as many preliminary clusters as colors in colorSpaceMap
        auto colorSpace = getImageDataXRGB888(colorSpaceMap).first;
        ColorFit<Color::XRGB888> colorFit(colorSpace);
        std::cout << "Color space has " << colorSpace.size() << " colors" << std::endl;
        // sort histogram colors into closest clusters in parallel
        std::cout << "Sorting colors into clusters... (this might take some time)" << std::endl;
        auto colorMap = colorFit.reduceColors(histogram, nrOfColors);
        // convert clusters to color map
        std::cout << "Building color table..." << std::endl;
        Magick::Image colorMapImage(Magick::Geometry(colorMap.size(), 1), "black");
        colorMapImage.type(Magick::ImageType::TrueColorType);
        colorMapImage.modifyImage();
        auto colorMapPixels = colorMapImage.getPixels(0, 0, colorMapImage.columns(), colorMapImage.rows());
        for (std::size_t i = 0; i < colorMap.size(); ++i)
        {
            *colorMapPixels++ = toMagick(colorMap[i]);
        }
        colorMapImage.syncPixels();
        // apply color map to all images
        std::cout << "Converting images..." << std::endl;
        std::vector<Data> result;
        std::transform(data.cbegin(), data.cend(), std::back_inserter(result), [&colorMapImage](const auto &d)
                       {
            // convert image to paletted using dithering
            auto temp = d.image;
            temp.quantizeDither(true);
            temp.quantizeDitherMethod(Magick::DitherMethod::FloydSteinbergDitherMethod);
            temp.map(colorMapImage, true);
            //temp.type(Magick::ImageType::PaletteType);
            // get image data and color map
            auto imageData = getImageData(temp);
            auto imageColorMap = getColorMap(temp);
            //REQUIRE(imageData.second == Color::Format::Paletted8, std::runtime_error, "Expected 8-bit paletted image");
            //auto dumpPath = std::filesystem::current_path() / "result" / (std::to_string(d.index) + ".png");
            //temp.write(dumpPath.c_str());
            return Data{d.index, d.fileName, temp.type(), DataSize{temp.size().width(), temp.size().height()}, DataType::Bitmap, imageData.second, {}, imageData.first, imageColorMap, Color::Format::Unknown, {}}; });
        return result;
    }

    Data Processing::toTruecolor(const InputData &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<std::string>(parameters.front()), std::runtime_error, "toTruecolor expects a single std::string parameter");
        const auto formatString = std::get<std::string>(parameters.front());
        Color::Format format = Color::Format::Unknown;
        if (formatString == "RGB888")
        {
            format = Color::Format::RGB888;
        }
        else if (formatString == "RGB565")
        {
            format = Color::Format::RGB565;
        }
        else if (formatString == "RGB555")
        {
            format = Color::Format::RGB555;
        }
        REQUIRE(format == Color::Format::RGB555 || format == Color::Format::RGB565 || format == Color::Format::RGB888, std::runtime_error, "Color format must be in [RGB555, RGB565, RGB888]");
        // get image data
        Magick::Image temp = data.image;
        auto imageData = getImageData(temp);
        REQUIRE(imageData.second == Color::Format::RGB888, std::runtime_error, "Expected RGB888 image");
        auto colorData = imageData.first;
        // convert colors if needed
        if (format == Color::Format::RGB555)
        {
            colorData = toRGB555(colorData);
        }
        else if (format == Color::Format::RGB565)
        {
            colorData = toRGB565(colorData);
        }
        return {data.index, data.fileName, temp.type(), {temp.size().width(), temp.size().height()}, DataType::Bitmap, format, {}, colorData, {}, Color::Format::Unknown, {}};
    }

    // ----------------------------------------------------------------------------

    Data Processing::toUniqueTileMap(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "toUniqueTileMap expects bitmaps as input data");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<bool>(parameters.front()), std::runtime_error, "toUniqueTileMap expects a single bool detect flips parameter");
        const auto detectFlips = std::get<bool>(parameters.front());
        auto result = image;
        auto screenAndTileMap = buildUniqueTileMap(image.data, image.size.width(), image.size.height(), bitsPerPixelForFormat(image.colorFormat), detectFlips);
        result.mapData = screenAndTileMap.first;
        result.data = screenAndTileMap.second;
        result.dataType = DataType::Tilemap;
        return result;
    }

    Data Processing::toTiles(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "toTiles expects bitmaps as input data");
        auto result = image;
        result.data = convertToTiles(image.data, image.size.width(), image.size.height(), bitsPerPixelForFormat(image.colorFormat));
        return result;
    }

    Data Processing::toSprites(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "toSprites expects bitmaps as input data");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "toSprites expects a single uint32_t sprite width parameter");
        const auto spriteWidth = std::get<uint32_t>(parameters.front());
        // convert image to sprites
        if (image.size.width() != spriteWidth)
        {
            auto result = image;
            result.data = convertToWidth(image.data, result.size.width(), result.size.height(), bitsPerPixelForFormat(result.colorFormat), spriteWidth);
            result.size = {spriteWidth, (result.size.width() * result.size.height()) / spriteWidth};
            return result;
        }
        return image;
    }

    Data Processing::addColor0(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "addColor0 expects bitmaps as input data");
        REQUIRE(image.colorFormat == Color::Format::Paletted8, std::runtime_error, "Adding a color can only be done for paletted images");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<Magick::Color>(parameters.front()), std::runtime_error, "addColor0 expects a single Color parameter");
        const auto color0 = std::get<Magick::Color>(parameters.front());
        // checkl of space in color map
        REQUIRE(image.colorMap.size() <= 255, std::runtime_error, "No space in color map (image has " << image.colorMap.size() << " colors)");
        // add color at front of color map
        auto result = image;
        result.data = incImageIndicesBy1(image.data);
        result.colorMap = addColorAtIndex0(image.colorMap, color0);
        result.colorMapFormat = Color::Format::Unknown;
        result.colorMapData = {};
        return result;
    }

    Data Processing::moveColor0(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "moveColor0 expects bitmaps as input data");
        REQUIRE(image.colorFormat == Color::Format::Paletted8, std::runtime_error, "Moving a color can only be done for paletted images");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<Magick::Color>(parameters.front()), std::runtime_error, "moveColor0 expects a single Color parameter");
        const auto color0 = std::get<Magick::Color>(parameters.front());
        // try to find color in palette
        auto oldColorIt = std::find(image.colorMap.begin(), image.colorMap.end(), color0);
        REQUIRE(oldColorIt != image.colorMap.end(), std::runtime_error, "Color " << asHex(color0) << " not found in image color map");
        const size_t oldIndex = std::distance(image.colorMap.begin(), oldColorIt);
        // check if index needs to move
        if (oldIndex != 0)
        {
            auto result = image;
            result.colorMapFormat = Color::Format::Unknown;
            result.colorMapData = {};
            // move index in color map and image data
            std::swap(result.colorMap[oldIndex], result.colorMap[0]);
            result.data = swapIndexToIndex0(image.data, oldIndex);
            return result;
        }
        return image;
    }

    Data Processing::reorderColors(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "reorderColors expects bitmaps as input data");
        REQUIRE(image.colorFormat == Color::Format::Paletted4 || image.colorFormat == Color::Format::Paletted8, std::runtime_error, "Reordering colors can only be done for paletted images");
        const auto newOrder = minimizeColorDistance(image.colorMap);
        auto result = image;
        result.data = swapIndices(image.data, newOrder);
        result.colorMap = swapColors(image.colorMap, newOrder);
        result.colorMapFormat = Color::Format::Unknown;
        result.colorMapData = {};
        return result;
    }

    Data Processing::shiftIndices(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "shiftIndices expects bitmaps as input data");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "shiftIndices expects a single uint32_t shift parameter");
        const auto shiftBy = std::get<uint32_t>(parameters.front());
        auto maxIndex = *std::max_element(image.data.cbegin(), image.data.cend());
        REQUIRE(maxIndex + shiftBy <= 255, std::runtime_error, "Max. index value in image is " << maxIndex << ", shift is " << shiftBy << "! Resulting index values would be > 255");
        Data result = image;
        std::for_each(result.data.begin(), result.data.end(), [shiftBy](auto &index)
                      { index = (index == 0) ? 0 : (((index + shiftBy) > 255) ? 255 : (index + shiftBy)); });
        return result;
    }

    Data Processing::pruneIndices(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "pruneIndices expects bitmaps as input data");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "pruneIndices expects a single uint32_t bit depth parameter");
        const auto bitDepth = std::get<uint32_t>(parameters.front());
        REQUIRE(bitDepth == 1 || bitDepth == 2 || bitDepth == 4, std::runtime_error, "Bit depth must be in [1, 2, 4]");
        REQUIRE(image.colorFormat == Color::Format::Paletted8, std::runtime_error, "Index pruning only possible for 8bit paletted images");
        REQUIRE(image.colorMap.size() <= 16, std::runtime_error, "Index pruning only possible for images with <= 16 colors");
        auto result = image;
        uint8_t maxIndex = *std::max_element(image.data.cbegin(), image.data.cend());
        if (bitDepth == 1)
        {
            REQUIRE(maxIndex == 1, std::runtime_error, "Index pruning to 1 bit only possible with index data <= 1");
            result.colorFormat = Color::Format::Paletted1;
            result.data = convertDataTo1Bit(image.data);
        }
        else if (bitDepth == 2)
        {
            REQUIRE(maxIndex < 4, std::runtime_error, "Index pruning to 2 bit only possible with index data <= 3");
            result.colorFormat = Color::Format::Paletted2;
            result.data = convertDataTo2Bit(image.data);
        }
        else
        {
            REQUIRE(maxIndex < 16, std::runtime_error, "Index pruning to 4 bit only possible with index data <= 15");
            result.colorFormat = Color::Format::Paletted4;
            result.data = convertDataTo4Bit(image.data);
        }
        return result;
    }

    Data Processing::toDelta8(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        auto result = image;
        result.data = deltaEncode(image.data);
        return result;
    }

    Data Processing::toDelta16(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        auto result = image;
        result.data = convertTo<uint8_t>(deltaEncode(convertTo<uint16_t>(image.data)));
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::compressLZ10(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<bool>(parameters.front()), std::runtime_error, "compressLZ10 expects a single bool VRAMcompatible parameter");
        const auto vramCompatible = std::get<bool>(parameters.front());
        // compress data
        auto result = image;
        result.data = Compression::compressLzss(image.data, vramCompatible, false);
        // result.data = LZSS::encodeLZSS(image.data, vramCompatible);
        return result;
    }

    Data Processing::compressLZ11(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<bool>(parameters.front()), std::runtime_error, "compressLZ11 expects a single bool VRAMcompatible parameter");
        const auto vramCompatible = std::get<bool>(parameters.front());
        // compress data
        auto result = image;
        result.data = Compression::compressLzss(image.data, vramCompatible, true);
        return result;
    }

    Data Processing::compressRLE(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<bool>(parameters.front()), std::runtime_error, "compressRLE expects a single bool VRAMcompatible parameter");
        const auto vramCompatible = std::get<bool>(parameters.front());
        // compress data
        auto result = image;
        // result.data = RLE::encodeRLE(image.data, vramCompatible);
        return result;
    }

    Data Processing::compressDXTG(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "compressDXTG expects bitmaps as input data");
        REQUIRE(image.colorFormat == Color::Format::RGB888 || image.colorFormat == Color::Format::RGB555, std::runtime_error, "DXTG compression is only possible for RGB888 and RGB555 truecolor images");
        REQUIRE(image.size.width() % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
        REQUIRE(image.size.height() % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT compression");
        // convert RGB888 to RGB565
        auto data = image.data;
        if (image.colorFormat == Color::Format::RGB888)
        {
            data = toRGB555(data);
        }
        auto result = image;
        result.colorFormat = Color::Format::RGB555;
        result.mapData = {};
        result.data = DXT::encodeDXTG(convertTo<uint16_t>(data), image.size.width(), image.size.height());
        result.colorMap = {};
        result.colorMapFormat = Color::Format::Unknown;
        result.colorMapData = {};
        return result;
    }

    Data Processing::compressDXTV(const Data &image, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "compressDXTV expects bitmaps as input data");
        REQUIRE(image.colorFormat == Color::Format::RGB888 || image.colorFormat == Color::Format::RGB555, std::runtime_error, "DXTV compression is only possible for RGB888 and RGB555 truecolor images");
        REQUIRE(image.size.width() % 16 == 0, std::runtime_error, "Image width must be a multiple of 16 for DXT compression");
        REQUIRE(image.size.height() % 16 == 0, std::runtime_error, "Image height must be a multiple of 16 for DXT compression");
        // get parameter(s)
        REQUIRE(parameters.size() == 2, std::runtime_error, "compressDXTV expects 2 double parameters");
        REQUIRE(std::holds_alternative<double>(parameters.at(0)), std::runtime_error, "compressDXTV keyframe interval must be a double");
        auto keyFrameInterval = static_cast<int32_t>(std::get<double>(parameters.at(0)));
        REQUIRE(keyFrameInterval >= 0 && keyFrameInterval <= 60, std::runtime_error, "compressDXTV keyframe interval must be in [0,60] (0 = none)");
        REQUIRE(std::holds_alternative<double>(parameters.at(1)), std::runtime_error, "compressDXTV max. block error must be a double");
        auto maxBlockError = std::get<double>(parameters.at(1));
        REQUIRE(maxBlockError >= 0.01 && maxBlockError <= 1, std::runtime_error, "compressDXTV max. block error must be in [0.01,1]");
        // convert RGB888 to RGB555
        auto data = image.data;
        if (image.colorFormat == Color::Format::RGB888)
        {
            data = toRGB555(data);
        }
        // check if needs to be a keyframe
        const bool isKeyFrame = keyFrameInterval > 0 ? ((image.index % keyFrameInterval) == 0 || state.empty()) : false;
        // compress data
        auto result = image;
        result.colorFormat = Color::Format::RGB555;
        result.mapData = {};
        auto dxtData = DXTV::encodeDXTV(convertTo<uint16_t>(data), state.empty() ? std::vector<uint16_t>() : convertTo<uint16_t>(state), image.size.width(), image.size.height(), isKeyFrame, maxBlockError);
        result.data = dxtData.first;
        result.colorMap = {};
        result.colorMapFormat = Color::Format::Unknown;
        result.colorMapData = {};
        // store decompressed image as state
        state = convertTo<uint8_t>(dxtData.second);
        // add statistics
        if (statistics != nullptr)
        {
            statistics->addImage("DXTV output", state, result.colorFormat, result.size.width(), result.size.height());
        }
        return result;
    }

    Data Processing::compressGVID(const Data &image, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Container::SPtr statistics)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "compressGVID expects bitmaps as input data");
        REQUIRE(image.colorFormat == Color::Format::RGB888, std::runtime_error, "GVID compression is only possible for RGB888 truecolor images");
        REQUIRE(image.size.width() % 16 == 0, std::runtime_error, "Image width must be a multiple of 16 for GVID compression");
        REQUIRE(image.size.height() % 16 == 0, std::runtime_error, "Image height must be a multiple of 16 for GVID compression");
        auto result = image;
        result.colorFormat = Color::Format::RGB888;
        result.mapData = {};
        result.data = GVID::encodeGVID(image.data, image.size.width(), image.size.height(), true);
        result.colorMap = {};
        result.colorMapFormat = Color::Format::Unknown;
        result.colorMapData = {};
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::padImageData(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padImageData expects a single uint32_t pad modulo parameter");
        auto multipleOf = std::get<uint32_t>(parameters.front());
        // pad data
        auto result = image;
        result.mapData = fillUpToMultipleOf(image.mapData, multipleOf / 2);
        result.data = fillUpToMultipleOf(image.data, multipleOf);
        return result;
    }

    Data Processing::padColorMap(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padColorMap expects a single uint32_t pad modulo parameter");
        auto multipleOf = std::get<uint32_t>(parameters.front());
        // pad data
        auto result = image;
        result.colorMap = fillUpToMultipleOf(image.colorMap, multipleOf);
        result.colorMapFormat = Color::Format::Unknown;
        result.colorMapData = {};
        return result;
    }

    Data Processing::convertColorMap(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<Color::Format>(parameters.front()), std::runtime_error, "convertColorMap expects a single Color::Format parameter");
        auto format = std::get<Color::Format>(parameters.front());
        REQUIRE(format == Color::Format::RGB555 || format == Color::Format::RGB565 || format == Color::Format::RGB888, std::runtime_error, "convertColorMap expects 15, 16 or 24 bit color formats");
        // convert colormap
        auto result = image;
        result.colorMapFormat = format;
        result.colorMapData = {};
        switch (format)
        {
        case Color::Format::RGB555:
            result.colorMapData = convertTo<uint8_t>(convertToBGR555(image.colorMap));
            break;
        case Color::Format::RGB565:
            result.colorMapData = convertTo<uint8_t>(convertToBGR565(image.colorMap));
            break;
        case Color::Format::RGB888:
            result.colorMapData = convertToBGR888(image.colorMap);
            break;
        default:
            THROW(std::runtime_error, "Bad target color map format");
        }
        return result;
    }

    Data Processing::padColorMapData(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padColorMapData expects a single uint32_t pad modulo parameter");
        auto multipleOf = std::get<uint32_t>(parameters.front());
        // pad raw color map data
        auto result = image;
        result.colorMapData = fillUpToMultipleOf(image.colorMapData, multipleOf);
        return result;
    }

    std::vector<Data> Processing::equalizeColorMaps(const std::vector<Data> &images, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics)
    {
        auto allColorMapsSameSize = std::find_if_not(images.cbegin(), images.cend(), [refSize = images.front().colorMap.size()](const auto &img)
                                                     { return img.colorMap.size() == refSize; }) == images.cend();
        // padd data if necessary
        if (!allColorMapsSameSize)
        {
            uint32_t maxColorMapColors = std::max_element(images.cbegin(), images.cend(), [](const auto &imgA, const auto &imgB)
                                                          { return imgA.colorMap.size() < imgB.colorMap.size(); })
                                             ->colorMap.size();
            std::vector<Data> result;
            std::transform(images.begin(), images.end(), std::back_inserter(result), [maxColorMapColors, statistics](auto &img)
                           { return padColorMap(img, {Parameter(maxColorMapColors)}, statistics); });
            return result;
        }
        return images;
    }

    Data Processing::imageDiff(const Data &image, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Container::SPtr statistics)
    {
        // check if a usable state was passed
        if (!state.empty())
        {
            // ok. calculate difference
            REQUIRE(image.data.size() == state.size(), std::runtime_error, "Images must have the same size");
            std::vector<uint8_t> diff(image.data.size());
            for (std::size_t i = 0; i < image.data.size(); i++)
            {
                diff[i] = state[i] - image.data[i];
            }
            // set current image to state
            state = image.data;
            auto result = image;
            result.data = diff;
            return result;
        }
        // set current image to state
        state = image.data;
        // no state, return input data
        return image;
    }

    void Processing::setStatisticsContainer(Statistics::Container::SPtr c)
    {
        m_statistics = c;
    }

    void Processing::addStep(ProcessingType type, const std::vector<Parameter> &parameters, bool prependProcessing, bool addStatistics)
    {
        m_steps.push_back({type, parameters, prependProcessing, addStatistics});
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
                else if (std::holds_alternative<Magick::Color>(p))
                {
                    result += asHex(std::get<Magick::Color>(p));
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<Color::Format>(p))
                {
                    result += to_string(std::get<Color::Format>(p));
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

    Data prependProcessing(const Data &img, uint32_t size, ProcessingType type, bool isFinal)
    {
        REQUIRE(img.data.size() < (1 << 24), std::runtime_error, "Data size stored must be < 16MB");
        REQUIRE(static_cast<uint32_t>(type) <= 127, std::runtime_error, "Type value must be <= 127");
        const uint32_t sizeAndType = ((size & 0xFFFFFF) << 8) | ((static_cast<uint32_t>(type) & 0x7F) | (isFinal ? static_cast<uint32_t>(ProcessingTypeFinal) : 0));
        auto result = img;
        result.data = prependValue(img.data, sizeAndType);
        return result;
    }

    std::vector<Data> Processing::processBatch(const std::vector<InputData> &data)
    {
        REQUIRE(data.size() > 0, std::runtime_error, "Empty data passed to processing");
        const auto firstFunctionType = ProcessingFunctions.find(m_steps.front().type)->second.type;
        REQUIRE(firstFunctionType == OperationType::Input || firstFunctionType == OperationType::BatchInput, std::runtime_error, "First step must be an input step");
        bool finalStepFound = false;
        std::vector<Data> processed;
        for (auto stepIt = m_steps.begin(); stepIt != m_steps.end(); ++stepIt)
        {
            auto stepStatistics = stepIt->addStatistics ? m_statistics : nullptr;
            auto &stepFunc = ProcessingFunctions.find(stepIt->type)->second;
            // check if this was the final processing step (first non-input processing)
            const bool isInputStep = stepFunc.type == OperationType::Input || stepFunc.type == OperationType::BatchInput;
            bool isFinalStep = false;
            if (!finalStepFound)
            {
                isFinalStep = !isInputStep;
                finalStepFound = isFinalStep;
            }
            if (stepFunc.type == OperationType::Input)
            {
                REQUIRE(processed.empty(), std::runtime_error, "Only a single input step allowed");
                auto inputFunc = std::get<InputFunc>(stepFunc.func);
                uint32_t index = 0;
                for (const auto &d : data)
                {
                    auto data = inputFunc(d, stepIt->parameters, stepStatistics);
                    processed.push_back(data);
                }
            }
            else if (stepFunc.type == OperationType::BatchInput)
            {
                REQUIRE(processed.empty(), std::runtime_error, "Only a single input step allowed");
                auto inputFunc = std::get<BatchInputFunc>(stepFunc.func);
                processed = inputFunc(data, stepIt->parameters, stepStatistics);
                std::for_each(processed.begin(), processed.end(), [index = 0](auto &p) mutable
                              { p.index = index++; });
            }
            else if (stepFunc.type == OperationType::Convert)
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc.func);
                for (auto &img : processed)
                {
                    const uint32_t inputSize = img.data.size();
                    img = convertFunc(img, stepIt->parameters, stepStatistics);
                    if (stepIt->prependProcessing)
                    {
                        img = prependProcessing(img, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = img.data.size() + sizeof(uint32_t);
                    img.maxMemoryNeeded = (!isInputStep && img.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : img.maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::ConvertState)
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc.func);
                for (auto &img : processed)
                {
                    const uint32_t inputSize = img.data.size();
                    img = convertFunc(img, stepIt->parameters, stepIt->state, stepStatistics);
                    if (stepIt->prependProcessing)
                    {
                        img = prependProcessing(img, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = img.data.size() + sizeof(uint32_t);
                    img.maxMemoryNeeded = (!isInputStep && img.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : img.maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::BatchConvert)
            {
                // get all input sizes
                std::vector<uint32_t> inputSizes = {};
                std::transform(processed.cbegin(), processed.cend(), std::back_inserter(inputSizes), [](const auto &d)
                               { return d.data.size(); });
                auto batchFunc = std::get<BatchConvertFunc>(stepFunc.func);
                processed = batchFunc(processed, stepIt->parameters, stepStatistics);
                for (auto pIt = processed.begin(); pIt != processed.end(); pIt++)
                {
                    if (stepIt->prependProcessing)
                    {
                        const uint32_t inputSize = inputSizes.at(std::distance(processed.begin(), pIt));
                        *pIt = prependProcessing(*pIt, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = pIt->data.size() + sizeof(uint32_t);
                    pIt->maxMemoryNeeded = (!isInputStep && pIt->maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : pIt->maxMemoryNeeded;
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

    Data Processing::processStream(const Magick::Image &image, uint32_t index)
    {
        REQUIRE(ProcessingFunctions.find(m_steps.front().type)->second.type == OperationType::Input, std::runtime_error, "First step must be an input step");
        bool finalStepFound = false;
        Data processed;
        for (auto stepIt = m_steps.begin(); stepIt != m_steps.end(); ++stepIt)
        {
            const uint32_t inputSize = processed.data.size();
            auto stepStatistics = stepIt->addStatistics ? m_statistics : nullptr;
            auto &stepFunc = ProcessingFunctions.find(stepIt->type)->second;
            if (stepFunc.type == OperationType::Input)
            {
                auto inputFunc = std::get<InputFunc>(stepFunc.func);
                processed = inputFunc({index, "", image}, stepIt->parameters, stepStatistics);
            }
            else if (stepFunc.type == OperationType::Convert)
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc.func);
                processed = convertFunc(processed, stepIt->parameters, stepStatistics);
            }
            else if (stepFunc.type == OperationType::ConvertState)
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc.func);
                processed = convertFunc(processed, stepIt->parameters, stepIt->state, stepStatistics);
            }
            // check if this was the final processing step (first non-input processing)
            bool isFinalStep = false;
            if (!finalStepFound)
            {
                isFinalStep = stepFunc.type != OperationType::Input;
                finalStepFound = isFinalStep;
            }
            // we're silently ignoring OperationType::BatchInput ::BatchConvert and ::Reduce operations here
            if (stepIt->prependProcessing)
            {
                processed = prependProcessing(processed, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
            }
            // record max. memory needed for everything, but the first step
            auto chunkMemoryNeeded = processed.data.size() + sizeof(uint32_t);
            processed.maxMemoryNeeded = (stepFunc.type != OperationType::Input && processed.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : processed.maxMemoryNeeded;
        }
        return processed;
    }
}
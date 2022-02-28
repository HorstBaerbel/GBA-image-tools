#include "imageprocessing.h"

#include "codec_dxt.h"
#include "codec_gvid.h"
#include "codec_rle.h"
#include "codec_lzss.h"
#include "colorhelpers.h"
#include "compresshelpers.h"
#include "datahelpers.h"
#include "exception.h"
#include "imagehelpers.h"
#include "spritehelpers.h"

#include <iostream>

namespace Image
{

    const std::map<ProcessingType, Processing::ProcessingFunc>
        Processing::ProcessingFunctions = {
            {ProcessingType::InputBlackWhite, {"binary", OperationType::Input, FunctionType(toBlackWhite)}},
            {ProcessingType::InputPaletted, {"paletted", OperationType::Input, FunctionType(toPaletted)}},
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
            {ProcessingType::CompressRLE, {"compress RLE", OperationType::Convert, FunctionType(compressRLE)}},
            {ProcessingType::CompressDXTG, {"compress DXTG", OperationType::Convert, FunctionType(compressDXTG)}},
            {ProcessingType::CompressGVID, {"compress GVID", OperationType::ConvertState, FunctionType(compressGVID)}},
            {ProcessingType::PadImageData, {"pad image data", OperationType::Convert, FunctionType(padImageData)}},
            {ProcessingType::PadColorMap, {"pad color map", OperationType::Convert, FunctionType(padColorMap)}},
            {ProcessingType::ConvertColorMap, {"convert color map", OperationType::Convert, FunctionType(convertColorMap)}},
            {ProcessingType::PadColorMapData, {"pad color map data", OperationType::Convert, FunctionType(padColorMapData)}},
            {ProcessingType::EqualizeColorMaps, {"equalize color maps", OperationType::BatchConvert, FunctionType(equalizeColorMaps)}},
            {ProcessingType::DeltaImage, {"image diff", OperationType::ConvertState, FunctionType(imageDiff)}}};

    Data Processing::toBlackWhite(const Magick::Image &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<float>(parameters.front()), std::runtime_error, "toBlackWhite expects a single float threshold parameter");
        const auto threshold = std::get<float>(parameters.front());
        REQUIRE(threshold >= 0 && threshold <= 1, std::runtime_error, "Threshold must be in [0.0, 1.0]");
        // threshold image
        Magick::Image temp = image;
        temp.threshold(Magick::Color::scaleDoubleToQuantum(threshold));
        temp.quantizeDither(false);
        temp.quantizeColors(2);
        temp.type(Magick::ImageType::PaletteType);
        // get image data and color map
        return {0, "", temp.type(), image.size(), DataType::Bitmap, ColorFormat::Paletted8, {}, getImageData(temp), getColorMap(temp), ColorFormat::Unknown, {}};
    }

    Data Processing::toPaletted(const Magick::Image &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 2 && std::holds_alternative<Magick::Image>(parameters.front()) && std::holds_alternative<uint32_t>(parameters.back()), std::runtime_error, "toPaletted expects a Magick::Image colorSpaceMap and uint32_t nrOfColors parameter");
        const auto nrOfcolors = std::get<uint32_t>(parameters.back());
        REQUIRE(nrOfcolors >= 2 && nrOfcolors <= 256, std::runtime_error, "Number of colors must be in [2, 256]");
        const auto colorSpaceMap = std::get<Magick::Image>(parameters.front());
        // map image to GBA color map, no dithering
        Magick::Image temp = image;
        temp.map(colorSpaceMap, false);
        // convert image to paletted
        temp.quantizeDither(true);
        temp.quantizeDitherMethod(Magick::DitherMethod::RiemersmaDitherMethod);
        temp.quantizeColors(nrOfcolors);
        temp.type(Magick::ImageType::PaletteType);
        // get image data and color map
        return {0, "", temp.type(), image.size(), DataType::Bitmap, ColorFormat::Paletted8, {}, getImageData(temp), getColorMap(temp), ColorFormat::Unknown, {}};
    }

    Data Processing::toTruecolor(const Magick::Image &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<std::string>(parameters.front()), std::runtime_error, "toTruecolor expects a single std::string parameter");
        const auto formatString = std::get<std::string>(parameters.front());
        ColorFormat format = ColorFormat::Unknown;
        if (formatString == "RGB888")
        {
            format = ColorFormat::RGB888;
        }
        else if (formatString == "RGB565")
        {
            format = ColorFormat::RGB565;
        }
        else if (formatString == "RGB555")
        {
            format = ColorFormat::RGB555;
        }
        REQUIRE(format == ColorFormat::RGB555 || format == ColorFormat::RGB565 || format == ColorFormat::RGB888, std::runtime_error, "Color format must be in [RGB555, RGB565, RGB888]");
        // get image data
        Magick::Image temp = image;
        auto imageData = getImageData(temp);
        // convert colors if needed
        if (format == ColorFormat::RGB555)
        {
            imageData = toRGB555(imageData);
        }
        else if (format == ColorFormat::RGB565)
        {
            imageData = toRGB565(imageData);
        }
        return {0, "", temp.type(), image.size(), DataType::Bitmap, format, {}, imageData, {}, ColorFormat::Unknown, {}};
    }

    // ----------------------------------------------------------------------------

    Data Processing::toUniqueTileMap(const Data &image, const std::vector<Parameter> &parameters)
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

    Data Processing::toTiles(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "toTiles expects bitmaps as input data");
        auto result = image;
        result.data = convertToTiles(image.data, image.size.width(), image.size.height(), bitsPerPixelForFormat(image.colorFormat));
        return result;
    }

    Data Processing::toSprites(const Data &image, const std::vector<Parameter> &parameters)
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
            result.size = Magick::Geometry(spriteWidth, (result.size.width() * result.size.height()) / spriteWidth);
            return result;
        }
        return image;
    }

    Data Processing::addColor0(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "addColor0 expects bitmaps as input data");
        REQUIRE(image.colorFormat == ColorFormat::Paletted8, std::runtime_error, "Adding a color can only be done for paletted images");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<Magick::Color>(parameters.front()), std::runtime_error, "addColor0 expects a single Color parameter");
        const auto color0 = std::get<Magick::Color>(parameters.front());
        // checkl of space in color map
        REQUIRE(image.colorMap.size() <= 255, std::runtime_error, "No space in color map (image has " << image.colorMap.size() << " colors)");
        // add color at front of color map
        auto result = image;
        result.data = incImageIndicesBy1(image.data);
        result.colorMap = addColorAtIndex0(image.colorMap, color0);
        result.colorMapFormat = ColorFormat::Unknown;
        result.colorMapData = {};
        return result;
    }

    Data Processing::moveColor0(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "moveColor0 expects bitmaps as input data");
        REQUIRE(image.colorFormat == ColorFormat::Paletted8, std::runtime_error, "Moving a color can only be done for paletted images");
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
            result.colorMapFormat = ColorFormat::Unknown;
            result.colorMapData = {};
            // move index in color map and image data
            std::swap(result.colorMap[oldIndex], result.colorMap[0]);
            result.data = swapIndexToIndex0(image.data, oldIndex);
            return result;
        }
        return image;
    }

    Data Processing::reorderColors(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "reorderColors expects bitmaps as input data");
        REQUIRE(image.colorFormat == ColorFormat::Paletted4 || image.colorFormat == ColorFormat::Paletted8, std::runtime_error, "Reordering colors can only be done for paletted images");
        const auto newOrder = minimizeColorDistance(image.colorMap);
        auto result = image;
        result.data = swapIndices(image.data, newOrder);
        result.colorMap = swapColors(image.colorMap, newOrder);
        result.colorMapFormat = ColorFormat::Unknown;
        result.colorMapData = {};
        return result;
    }

    Data Processing::shiftIndices(const Data &image, const std::vector<Parameter> &parameters)
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

    Data Processing::pruneIndices(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "pruneIndices expects bitmaps as input data");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "pruneIndices expects a single uint32_t bit depth parameter");
        const auto bitDepth = std::get<uint32_t>(parameters.front());
        REQUIRE(bitDepth == 1 || bitDepth == 2 || bitDepth == 4, std::runtime_error, "Bit depth must be in [1, 2, 4]");
        REQUIRE(image.colorFormat == ColorFormat::Paletted8, std::runtime_error, "Index pruning only possible for 8bit paletted images");
        REQUIRE(image.colorMap.size() <= 16, std::runtime_error, "Index pruning only possible for images with <= 16 colors");
        auto result = image;
        uint8_t maxIndex = std::max(*std::max_element(image.data.cbegin(), image.data.cend()), maxIndex);
        if (bitDepth == 1)
        {
            REQUIRE(maxIndex == 1, std::runtime_error, "Index pruning to 1 bit only possible with index data <= 1");
            result.colorFormat = ColorFormat::Paletted1;
            result.data = convertDataTo1Bit(image.data);
        }
        else if (bitDepth == 2)
        {
            REQUIRE(maxIndex < 4, std::runtime_error, "Index pruning to 2 bit only possible with index data <= 3");
            result.colorFormat = ColorFormat::Paletted2;
            result.data = convertDataTo2Bit(image.data);
        }
        else
        {
            REQUIRE(maxIndex < 16, std::runtime_error, "Index pruning to 4 bit only possible with index data <= 15");
            result.colorFormat = ColorFormat::Paletted4;
            result.data = convertDataTo4Bit(image.data);
        }
        return result;
    }

    Data Processing::toDelta8(const Data &image, const std::vector<Parameter> &parameters)
    {
        auto result = image;
        result.data = deltaEncode(image.data);
        return result;
    }

    Data Processing::toDelta16(const Data &image, const std::vector<Parameter> &parameters)
    {
        auto result = image;
        result.data = convertTo<uint8_t>(deltaEncode(convertTo<uint16_t>(image.data)));
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::compressLZ10(const Data &image, const std::vector<Parameter> &parameters)
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

    Data Processing::compressLZ11(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<bool>(parameters.front()), std::runtime_error, "compressLZ11 expects a single bool VRAMcompatible parameter");
        const auto vramCompatible = std::get<bool>(parameters.front());
        // compress data
        auto result = image;
        result.data = Compression::compressLzss(image.data, vramCompatible, true);
        return result;
    }

    Data Processing::compressRLE(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<bool>(parameters.front()), std::runtime_error, "compressRLE expects a single bool VRAMcompatible parameter");
        const auto vramCompatible = std::get<bool>(parameters.front());
        // compress data
        auto result = image;
        result.data = RLE::encodeRLE(image.data, vramCompatible);
        return result;
    }

    Data Processing::compressDXTG(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "compressDXTG expects bitmaps as input data");
        REQUIRE(image.colorFormat == ColorFormat::RGB888 || image.colorFormat == ColorFormat::RGB555, std::runtime_error, "DXTG compression is only possible for RGB888 and RGB555 truecolor images");
        REQUIRE(image.size.width() % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
        REQUIRE(image.size.height() % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT compression");
        // convert RGB888 to RGB565
        auto data = image.data;
        if (image.colorFormat == ColorFormat::RGB888)
        {
            data = toRGB555(data);
        }
        auto result = image;
        result.colorFormat = ColorFormat::RGB555;
        result.mapData = {};
        result.data = DXT::encodeDXTG(convertTo<uint16_t>(data), image.size.width(), image.size.height());
        result.colorMap = {};
        result.colorMapFormat = ColorFormat::Unknown;
        result.colorMapData = {};
        return result;
    }

    Data Processing::compressGVID(const Data &image, const std::vector<Parameter> &parameters, std::vector<Parameter> &state)
    {
        REQUIRE(image.dataType == DataType::Bitmap, std::runtime_error, "compressGVID expects bitmaps as input data");
        REQUIRE(image.colorFormat == ColorFormat::RGB888, std::runtime_error, "GVID compression is only possible for RGB888 truecolor images");
        REQUIRE(image.size.width() % 16 == 0, std::runtime_error, "Image width must be a multiple of 16 for GVID compression");
        REQUIRE(image.size.height() % 16 == 0, std::runtime_error, "Image height must be a multiple of 16 for GVID compression");
        auto result = image;
        result.colorFormat = ColorFormat::RGB888;
        result.mapData = {};
        result.data = GVID::encodeGVID(image.data, image.size.width(), image.size.height(), true);
        result.colorMap = {};
        result.colorMapFormat = ColorFormat::Unknown;
        result.colorMapData = {};
        return result;
    }

    // ----------------------------------------------------------------------------

    Data Processing::padImageData(const Data &image, const std::vector<Parameter> &parameters)
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

    Data Processing::padColorMap(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padColorMap expects a single uint32_t pad modulo parameter");
        auto multipleOf = std::get<uint32_t>(parameters.front());
        // pad data
        auto result = image;
        result.colorMap = fillUpToMultipleOf(image.colorMap, multipleOf);
        result.colorMapFormat = ColorFormat::Unknown;
        result.colorMapData = {};
        return result;
    }

    Data Processing::convertColorMap(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<ColorFormat>(parameters.front()), std::runtime_error, "convertColorMap expects a single ColorFormat parameter");
        auto format = std::get<ColorFormat>(parameters.front());
        REQUIRE(format == ColorFormat::RGB555 || format == ColorFormat::RGB565 || format == ColorFormat::RGB888, std::runtime_error, "convertColorMap expects 15, 16 or 24 bit color formats");
        // convert colormap
        auto result = image;
        result.colorMapFormat = format;
        result.colorMapData = {};
        switch (format)
        {
        case ColorFormat::RGB555:
            result.colorMapData = convertTo<uint8_t>(convertToBGR555(image.colorMap));
            break;
        case ColorFormat::RGB565:
            result.colorMapData = convertTo<uint8_t>(convertToBGR565(image.colorMap));
            break;
        case ColorFormat::RGB888:
            result.colorMapData = convertToBGR888(image.colorMap);
            break;
        default:
            THROW(std::runtime_error, "Bad target color map format");
        }
        return result;
    }

    Data Processing::padColorMapData(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padColorMapData expects a single uint32_t pad modulo parameter");
        auto multipleOf = std::get<uint32_t>(parameters.front());
        // pad raw color map data
        auto result = image;
        result.colorMapData = fillUpToMultipleOf(image.colorMapData, multipleOf);
        return result;
    }

    std::vector<Data> Processing::equalizeColorMaps(const std::vector<Data> &images, const std::vector<Parameter> &parameters)
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
            std::transform(images.begin(), images.end(), std::back_inserter(result), [maxColorMapColors](auto &img)
                           { return padColorMap(img, {Parameter(maxColorMapColors)}); });
            return result;
        }
        return images;
    }

    Data Processing::imageDiff(const Data &image, const std::vector<Parameter> &parameters, std::vector<Parameter> &state)
    {
        // check if a usable state was passed
        if (state.size() == 1 && std::holds_alternative<Data>(state.front()))
        {
            // ok. calculate difference
            auto &prevImage = std::get<Data>(state.front());
            REQUIRE(image.data.size() == prevImage.data.size(), std::runtime_error, "Images must have the same size");
            std::vector<uint8_t> diff(image.data.size());
            for (std::size_t i = 0; i < image.data.size(); i++)
            {
                diff[i] = prevImage.data[i] - image.data[i];
            }
            // set current image to state
            std::get<Data>(state.front()) = image;
            auto result = image;
            result.data = diff;
            return result;
        }
        // set current image to state
        state.push_back(image);
        // no state, return input data
        return image;
    }

    void Processing::addStep(ProcessingType type, const std::vector<Parameter> &parameters, bool prependProcessing)
    {
        m_steps.push_back({type, parameters, prependProcessing});
    }

    std::size_t Processing::size() const
    {
        return m_steps.size();
    }

    void Processing::clear()
    {
        m_steps.clear();
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
                result += std::holds_alternative<bool>(p) ? (std::get<bool>(p) ? "true" : "false") : "";
                result += std::holds_alternative<int32_t>(p) ? std::to_string(std::get<int32_t>(p)) : "";
                result += std::holds_alternative<uint32_t>(p) ? std::to_string(std::get<uint32_t>(p)) : "";
                result += std::holds_alternative<float>(p) ? std::to_string(std::get<float>(p)) : "";
                result += std::holds_alternative<Magick::Color>(p) ? asHex(std::get<Magick::Color>(p)) : "";
                result += std::holds_alternative<ColorFormat>(p) ? to_string(std::get<ColorFormat>(p)) : "";
                result += std::holds_alternative<std::string>(p) ? std::get<std::string>(p) : "";
                result += (pi < (step.parameters.size() - 1) ? " " : "");
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

    std::vector<Data> Processing::processBatch(const std::vector<Data> &data, bool clearState)
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
                isFinalStep = stepFunc.type != OperationType::Input;
                finalStepFound = isFinalStep;
            }
            // we're silently ignoring OperationType::Input operations here
            if (stepFunc.type == OperationType::Convert)
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc.func);
                for (auto &img : processed)
                {
                    const uint32_t inputSize = img.data.size();
                    img = convertFunc(img, stepIt->parameters);
                    if (stepIt->prependProcessing)
                    {
                        img = prependProcessing(img, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = img.data.size() + sizeof(uint32_t);
                    img.maxMemoryNeeded = (stepFunc.type != OperationType::Input && img.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : img.maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::ConvertState)
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc.func);
                for (auto &img : processed)
                {
                    const uint32_t inputSize = img.data.size();
                    img = convertFunc(img, stepIt->parameters, stepIt->state);
                    if (stepIt->prependProcessing)
                    {
                        img = prependProcessing(img, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    }
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = img.data.size() + sizeof(uint32_t);
                    img.maxMemoryNeeded = (stepFunc.type != OperationType::Input && img.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : img.maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::BatchConvert)
            {
                // get all input sizes
                std::vector<uint32_t> inputSizes = {};
                std::transform(processed.cbegin(), processed.cend(), std::back_inserter(inputSizes), [](const auto &d)
                               { return d.data.size(); });
                auto batchFunc = std::get<BatchConvertFunc>(stepFunc.func);
                processed = batchFunc(processed, stepIt->parameters);
                for (auto pIt = processed.begin(); pIt != processed.end(); pIt++)
                {
                    const uint32_t inputSize = inputSizes.at(std::distance(processed.begin(), pIt));
                    *pIt = prependProcessing(*pIt, static_cast<uint32_t>(inputSize), stepIt->type, isFinalStep);
                    // record max. memory needed for everything, but the first step
                    auto chunkMemoryNeeded = pIt->data.size() + sizeof(uint32_t);
                    pIt->maxMemoryNeeded = (stepFunc.type != OperationType::Input && pIt->maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : pIt->maxMemoryNeeded;
                }
            }
            else if (stepFunc.type == OperationType::Reduce)
            {
                auto reduceFunc = std::get<ReduceFunc>(stepFunc.func);
                processed = {reduceFunc(processed, stepIt->parameters)};
            }
        }
        return processed;
    }

    Data Processing::processStream(const Magick::Image &image, bool clearState)
    {
        REQUIRE(ProcessingFunctions.find(m_steps.front().type)->second.type == OperationType::Input, std::runtime_error, "First step must be an input step");
        bool finalStepFound = false;
        Data processed;
        for (auto stepIt = m_steps.begin(); stepIt != m_steps.end(); ++stepIt)
        {
            const uint32_t inputSize = processed.data.size();
            auto &stepFunc = ProcessingFunctions.find(stepIt->type)->second;
            if (stepFunc.type == OperationType::Input)
            {
                auto inputFunc = std::get<InputFunc>(stepFunc.func);
                processed = inputFunc(image, stepIt->parameters);
            }
            else if (stepFunc.type == OperationType::Convert)
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc.func);
                processed = convertFunc(processed, stepIt->parameters);
            }
            else if (stepFunc.type == OperationType::ConvertState)
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc.func);
                processed = convertFunc(processed, stepIt->parameters, stepIt->state);
            }
            // check if this was the final processing step (first non-input processing)
            bool isFinalStep = false;
            if (!finalStepFound)
            {
                isFinalStep = stepFunc.type != OperationType::Input;
                finalStepFound = isFinalStep;
            }
            // we're silently ignoring OperationType::BatchConvert and ::Reduce operations here
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
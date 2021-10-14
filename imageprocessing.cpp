#include "imageprocessing.h"

#include "exception.h"
#include "imagehelpers.h"
#include "colorhelpers.h"
#include "datahelpers.h"
#include "compresshelpers.h"
#include "spritehelpers.h"

#include <execution>

namespace Image
{

    const std::map<Processing::Type, Processing::ProcessingFunc>
        Processing::ProcessingFunctions = {
            {Type::InputBlackWhite, {"binary", OperationType::Input, FunctionType(toBlackWhite)}},
            {Type::InputPaletted, {"paletted", OperationType::Input, FunctionType(toPaletted)}},
            {Type::InputTruecolor, {"truecolor", OperationType::Input, FunctionType(toTruecolor)}},
            {Type::ConvertTiles, {"tiles", OperationType::Convert, FunctionType(toTiles)}},
            {Type::ConvertSprites, {"sprites", OperationType::Convert, FunctionType(toSprites)}},
            {Type::AddColor0, {"add color #0", OperationType::Convert, FunctionType(addColor0)}},
            {Type::MoveColor0, {"move color #0", OperationType::Convert, FunctionType(moveColor0)}},
            {Type::ReorderColors, {"reorder colors", OperationType::Convert, FunctionType(reorderColors)}},
            {Type::ShiftIndices, {"shift indices", OperationType::Convert, FunctionType(shiftIndices)}},
            {Type::PruneIndices, {"prune indices", OperationType::Convert, FunctionType(pruneIndices)}},
            {Type::ConvertDelta8, {"delta-8", OperationType::Convert, FunctionType(toDelta8)}},
            {Type::ConvertDelta16, {"delta-16", OperationType::Convert, FunctionType(toDelta16)}},
            {Type::CompressLz10, {"compress LZ10", OperationType::Convert, FunctionType(compressLZ10)}},
            {Type::CompressLz11, {"compress LZ11", OperationType::Convert, FunctionType(compressLZ11)}},
            {Type::CompressRLE, {"compress RLE", OperationType::Convert, FunctionType(compressRLE)}},
            {Type::CompressDXT1, {"compress DXT1", OperationType::Convert, FunctionType(compressDXT1)}},
            {Type::PadImageData, {"pad image data", OperationType::Convert, FunctionType(padImageData)}},
            {Type::PadColorMap, {"pad color map", OperationType::Convert, FunctionType(padColorMap)}},
            {Type::ConvertColorMap, {"convert color map", OperationType::Convert, FunctionType(convertColorMap)}},
            {Type::PadColorMapData, {"pad color map data", OperationType::Convert, FunctionType(padColorMapData)}},
            {Type::EqualizeColorMaps, {"equalize color maps", OperationType::BatchConvert, FunctionType(equalizeColorMaps)}},
            {Type::DeltaImage, {"image diff", OperationType::ConvertState, FunctionType(imageDiff)}}};

    std::vector<std::vector<uint8_t>> Processing::RGB565DistanceSqrCache;

    Data Processing::toBlackWhite(const Magick::Image &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<float>(parameters.front()), std::runtime_error, "toBlackWhite expects a single float threshold parameter");
        auto threshold = std::get<float>(parameters.front());
        REQUIRE(threshold >= 0 && threshold <= 1, std::runtime_error, "Threshold must be in [0.0, 1.0]");
        // threshold image
        Magick::Image temp = image;
        temp.threshold(Magick::Color::scaleDoubleToQuantum(threshold));
        temp.quantizeDither(false);
        temp.quantizeColors(2);
        temp.type(Magick::ImageType::PaletteType);
        // get image data and color map
        return {"", temp.type(), image.size(), ColorFormat::Paletted8, getImageData(temp), getColorMap(temp), ColorFormat::Unknown, {}};
    }

    Data Processing::toPaletted(const Magick::Image &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 2 && std::holds_alternative<Magick::Image>(parameters.front()) && std::holds_alternative<uint32_t>(parameters.back()), std::runtime_error, "toPaletted expects a Magick::Image colorSpaceMap and uint32_t nrOfColors parameter");
        auto nrOfcolors = std::get<uint32_t>(parameters.back());
        REQUIRE(nrOfcolors >= 2 && nrOfcolors <= 256, std::runtime_error, "Number of colors must be in [2, 256]");
        auto colorSpaceMap = std::get<Magick::Image>(parameters.front());
        // map image to GBA color map, no dithering
        Magick::Image temp = image;
        temp.map(colorSpaceMap, false);
        // convert image to paletted
        temp.quantizeDither(true);
        temp.quantizeDitherMethod(Magick::DitherMethod::RiemersmaDitherMethod);
        temp.quantizeColors(nrOfcolors);
        temp.type(Magick::ImageType::PaletteType);
        // get image data and color map
        return {"", temp.type(), image.size(), ColorFormat::Paletted8, getImageData(temp), getColorMap(temp), ColorFormat::Unknown, {}};
    }

    Data Processing::toTruecolor(const Magick::Image &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<std::string>(parameters.front()), std::runtime_error, "toTruecolor expects a single std::string parameter");
        auto formatString = std::get<std::string>(parameters.front());
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
        return {"", temp.type(), image.size(), format, imageData, {}, ColorFormat::Unknown, {}};
    }

    // ----------------------------------------------------------------------------

    Data Processing::toTiles(const Data &image, const std::vector<Parameter> &parameters)
    {
        Data result = {image.fileName, image.type, image.size, image.format, {}, image.colorMap, image.colorMapFormat, image.colorMapData};
        result.data = convertToTiles(image.data, image.size.width(), image.size.height(), bitsPerPixelForFormat(image.format));
        return result;
    }

    Data Processing::toSprites(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "toSprites expects a single uint32_t sprite width parameter");
        auto spriteWidth = std::get<uint32_t>(parameters.front());
        // convert image to sprites
        if (image.size.width() != spriteWidth)
        {
            Data result = {image.fileName, image.type, image.size, image.format, {}, image.colorMap, image.colorMapFormat, image.colorMapData};
            result.data = convertToWidth(image.data, result.size.width(), result.size.height(), bitsPerPixelForFormat(result.format), spriteWidth);
            result.size = Magick::Geometry(spriteWidth, (result.size.width() * result.size.height()) / spriteWidth);
            return result;
        }
        return image;
    }

    Data Processing::addColor0(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.format == ColorFormat::Paletted8, std::runtime_error, "Adding a color can only be done for paletted images");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<Magick::Color>(parameters.front()), std::runtime_error, "addColor0 expects a single Color parameter");
        auto color0 = std::get<Magick::Color>(parameters.front());
        // checkl of space in color map
        REQUIRE(image.colorMap.size() <= 255, std::runtime_error, "No space in color map (image has " << image.colorMap.size() << " colors)");
        // add color at front of color map
        Data result = {image.fileName, image.type, image.size, image.format, {}, {}, ColorFormat::Unknown, {}};
        result.colorMap = addColorAtIndex0(image.colorMap, color0);
        result.data = incImageIndicesBy1(image.data);
        return result;
    }

    Data Processing::moveColor0(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.format == ColorFormat::Paletted8, std::runtime_error, "Moving a color can only be done for paletted images");
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<Magick::Color>(parameters.front()), std::runtime_error, "moveColor0 expects a single Color parameter");
        auto color0 = std::get<Magick::Color>(parameters.front());
        // try to find color in palette
        auto oldColorIt = std::find(image.colorMap.begin(), image.colorMap.end(), color0);
        REQUIRE(oldColorIt != image.colorMap.end(), std::runtime_error, "Color " << asHex(color0) << " not found in image color map");
        const size_t oldIndex = std::distance(image.colorMap.begin(), oldColorIt);
        // check if index needs to move
        if (oldIndex != 0)
        {
            Data result = {image.fileName, image.type, image.size, image.format, {}, image.colorMap, ColorFormat::Unknown, {}};
            // move index in color map and image data
            std::swap(result.colorMap[oldIndex], result.colorMap[0]);
            result.data = swapIndexToIndex0(image.data, oldIndex);
            return result;
        }
        return image;
    }

    Data Processing::reorderColors(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.format == ColorFormat::Paletted4 || image.format == ColorFormat::Paletted8, std::runtime_error, "Reordering colors can only be done for paletted images");
        Data result = {image.fileName, image.type, image.size, image.format, {}, {}, ColorFormat::Unknown, {}};
        const auto newOrder = minimizeColorDistance(image.colorMap);
        result.colorMap = swapColors(image.colorMap, newOrder);
        result.data = swapIndices(image.data, newOrder);
        return result;
    }

    Data Processing::shiftIndices(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "shiftIndices expects a single uint32_t shift parameter");
        auto shiftBy = std::get<uint32_t>(parameters.front());
        auto maxIndex = *std::max_element(image.data.cbegin(), image.data.cend());
        REQUIRE(maxIndex + shiftBy <= 255, std::runtime_error, "Max. index value in image is " << maxIndex << ", shift is " << shiftBy << "! Resulting index values would be > 255");
        Data result = image;
        std::for_each(result.data.begin(), result.data.end(), [shiftBy](auto &index)
                      { index = (index == 0) ? 0 : (((index + shiftBy) > 255) ? 255 : (index + shiftBy)); });
        return result;
    }

    Data Processing::pruneIndices(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.format == ColorFormat::Paletted8, std::runtime_error, "Index pruning only possible for 8bit paletted images");
        REQUIRE(image.colorMap.size() <= 16, std::runtime_error, "Index pruning only possible for images with <= 16 colors");
        uint8_t maxIndex = std::max(*std::max_element(image.data.cbegin(), image.data.cend()), maxIndex);
        REQUIRE(maxIndex < 16, std::runtime_error, "Index pruning only possible with index data <= 15");
        if (maxIndex <= 1)
        {
            return {image.fileName, image.type, image.size, ColorFormat::Paletted1, convertDataTo1Bit(image.data), image.colorMap, image.colorMapFormat, image.colorMapData};
        }
        if (maxIndex > 1 && maxIndex <= 3)
        {
            return {image.fileName, image.type, image.size, ColorFormat::Paletted1, convertDataTo2Bit(image.data), image.colorMap, image.colorMapFormat, image.colorMapData};
        }
        else
        {
            return {image.fileName, image.type, image.size, ColorFormat::Paletted4, convertDataTo4Bit(image.data), image.colorMap, image.colorMapFormat, image.colorMapData};
        }
    }

    Data Processing::toDelta8(const Data &image, const std::vector<Parameter> &parameters)
    {
        Data result = {image.fileName, image.type, image.size, image.format, {}, image.colorMap, image.colorMapFormat, image.colorMapData};
        result.data = deltaEncode(image.data);
        return result;
    }

    Data Processing::toDelta16(const Data &image, const std::vector<Parameter> &parameters)
    {
        Data result = {image.fileName, image.type, image.size, image.format, {}, image.colorMap, image.colorMapFormat, image.colorMapData};
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
        Data result = {image.fileName, image.type, image.size, image.format, {}, image.colorMap, image.colorMapFormat, image.colorMapData};
        result.data = Compression::compressLzss(image.data, vramCompatible, false);
        return result;
    }

    Data Processing::compressLZ11(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<bool>(parameters.front()), std::runtime_error, "compressLZ11 expects a single bool VRAMcompatible parameter");
        const auto vramCompatible = std::get<bool>(parameters.front());
        // compress data
        Data result = {image.fileName, image.type, image.size, image.format, {}, image.colorMap, image.colorMapFormat, image.colorMapData};
        result.data = Compression::compressLzss(image.data, vramCompatible, true);
        return result;
    }

    Data Processing::compressRLE(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<bool>(parameters.front()), std::runtime_error, "compressRLE expects a single bool VRAMcompatible parameter");
        const auto vramCompatible = std::get<bool>(parameters.front());
        // compress data
        Data result = {image.fileName, image.type, image.size, image.format, {}, image.colorMap, image.colorMapFormat, image.colorMapData};
        result.data = Compression::compressRLE(image.data, vramCompatible);
        return result;
    }

    std::vector<uint8_t> Processing::encodeBlockDXT1(const uint16_t *start, uint32_t pixelsPerScanline, const std::vector<std::vector<uint8_t>> &distanceSqrMap)
    {
        REQUIRE(pixelsPerScanline % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
        // get block colors for all 16 pixels
        std::vector<uint16_t> colors(16);
        auto c16It = colors.begin();
        auto pixel = start;
        for (int y = 0; y < 4; y++)
        {
            *c16It++ = *pixel++;
            *c16It++ = *pixel++;
            *c16It++ = *pixel++;
            *c16It++ = *pixel++;
            pixel += pixelsPerScanline - 4;
        }
        // calculate minimum color distance for each combination of block endpoints
        uint32_t bestDistance = std::numeric_limits<uint32_t>::max();
        uint16_t bestC0 = colors.front();
        uint16_t bestC1 = colors.front();
        std::vector<uint16_t> bestIndices(16, 0);
        std::vector<uint16_t> iterationIndices(16);
        for (auto c0It = colors.cbegin(); c0It != colors.cend(); c0It++)
        {
            uint16_t endpoints[4] = {*c0It, 0, 0, 0};
            for (auto c1It = colors.cbegin(); c1It != colors.cend(); c1It++)
            {
                endpoints[1] = *c1It;
                // calculate intermediate colors c2 and c3
                endpoints[2] = lerpRGB565(*c0It, *c1It, 1.0 / 3.0);
                endpoints[3] = lerpRGB565(*c0It, *c1It, 2.0 / 3.0);
                // calculate minimum distance for all colors to endpoints
                uint32_t iterationDistance = 0;
                std::fill(iterationIndices.begin(), iterationIndices.end(), 0);
                for (uint32_t ci = 0; ci < 16; ci++)
                {
                    // calculate minimum distance for each index for this color
                    uint8_t colorDistance = std::numeric_limits<uint8_t>::max();
                    for (uint16_t ei = 0; ei < 4; ei++)
                    {
                        auto indexDistance = distanceSqrMap[ci][endpoints[ei]];
                        // check if result improved
                        if (indexDistance < colorDistance)
                        {
                            colorDistance = indexDistance;
                            iterationIndices[ci] = ei;
                        }
                    }
                    iterationDistance += colorDistance;
                }
                // check if result improved
                if (iterationDistance < bestDistance)
                {
                    bestDistance = iterationDistance;
                    bestC0 = *c0It;
                    bestC1 = *c1It;
                    bestIndices = iterationIndices;
                }
            }
        }
        // build result data
        std::vector<uint8_t> result(2 * 2 + 16 * 2 / 8);
        // add color endpoints c0 and c1
        auto data16 = reinterpret_cast<uint16_t *>(result.data());
        *data16++ = bestC0;
        *data16++ = bestC1;
        // add index data
        uint32_t indices = 0;
        for (auto iIt = bestIndices.cbegin(); iIt != bestIndices.cend(); iIt++)
        {
            indices = (indices << 2) | *iIt;
        }
        *(reinterpret_cast<uint32_t *>(data16)) = indices;
        return result;
    }

    Data Processing::compressDXT1(const Data &image, const std::vector<Parameter> &parameters)
    {
        REQUIRE(image.format == ColorFormat::RGB888 || image.format == ColorFormat::RGB565, std::runtime_error, "DXT compression is only possible for RGB888 and RGB565 truecolor images");
        REQUIRE(image.size.width() % 4 == 0, std::runtime_error, "Image width must be a multiple of 4 for DXT compression");
        REQUIRE(image.size.height() % 4 == 0, std::runtime_error, "Image height must be a multiple of 4 for DXT compression");
        // check if squared distance map has been allocated
        if (RGB565DistanceSqrCache.empty())
        {
            RGB565DistanceSqrCache = std::move(RGB565DistanceSqrTable());
        }
        // convert RGB888 to RGB565
        auto data = image.data;
        if (image.format == ColorFormat::RGB888)
        {
            data = toRGB565(data);
        }
        // convert data to uint16_t values
        const auto data16 = convertTo<uint16_t>(data);
        // build y-position table for parallel for
        std::vector<uint32_t> ys(image.size.height() / 4);
        std::generate(ys.begin(), ys.end(), [y = 0]() mutable
                      {
                          y += 4;
                          return y - 4;
                      });
        // compress to DXT1. we get 8 bytes per 4x4 block / 16 pixels
        const auto yStride = image.size.width() * 8 / 16;
        std::vector<uint8_t> resultData(image.size.width() * image.size.height() * 8 / 16);
        std::for_each(std::execution::par_unseq, ys.cbegin(), ys.cend(), [&](uint32_t y)
                      {
                          for (uint32_t x = 0; x < image.size.width(); x += 4)
                          {
                              auto block = encodeBlockDXT1(data16.data() + y * image.size.width() + x, image.size.width(), RGB565DistanceSqrCache);
                              std::copy(block.cbegin(), block.cend(), std::next(resultData.begin(), y * yStride + x * 8 / 4));
                          }
                      });
        return {image.fileName, image.type, image.size, ColorFormat::RGB565, resultData, {}, ColorFormat::Unknown, {}};
    }

    // ----------------------------------------------------------------------------

    Data Processing::padImageData(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padImageData expects a single uint32_t pad modulo parameter");
        auto multipleOf = std::get<uint32_t>(parameters.front());
        // pad data
        Data result = {image.fileName, image.type, image.size, image.format, {}, image.colorMap, image.colorMapFormat, image.colorMapData};
        result.data = fillUpToMultipleOf(image.data, multipleOf);
        return result;
    }

    Data Processing::padColorMap(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padColorMap expects a single uint32_t pad modulo parameter");
        auto multipleOf = std::get<uint32_t>(parameters.front());
        // pad data
        Data result = {image.fileName, image.type, image.size, image.format, image.data, {}, ColorFormat::Unknown, {}};
        result.colorMap = fillUpToMultipleOf(image.colorMap, multipleOf);
        return result;
    }

    Data Processing::convertColorMap(const Data &image, const std::vector<Parameter> &parameters)
    {
        // get parameter(s)
        REQUIRE(parameters.size() == 1 && std::holds_alternative<ColorFormat>(parameters.front()), std::runtime_error, "convertColorMap expects a single ColorFormat parameter");
        auto format = std::get<ColorFormat>(parameters.front());
        REQUIRE(format == ColorFormat::RGB555 || format == ColorFormat::RGB565 || format == ColorFormat::RGB888, std::runtime_error, "convertColorMap expects 15, 16 or 24 bit color formats");
        // convert colormap
        Data result = {image.fileName, image.type, image.size, image.format, image.data, image.colorMap, format, {}};
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
        Data result = {image.fileName, image.type, image.size, image.format, image.data, image.colorMap, image.colorMapFormat, {}};
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
            return {image.fileName, image.type, image.size, image.format, diff, image.colorMap, image.colorMapFormat, image.colorMapData};
        }
        // set current image to state
        state.push_back(image);
        // no state, return input data
        return image;
    }

    void Processing::addStep(Type type, const std::vector<Parameter> &parameters, bool prependProcessing)
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
                result += std::holds_alternative<std::string>(p) ? std::get<std::string>(p) : "";
                result += (pi < (step.parameters.size() - 1) ? " " : "");
            }
            result += (si < (m_steps.size() - 1) ? seperator : "");
        }
        return result;
    }

    Data prependProcessing(const Data &img, uint32_t size, Processing::Type type)
    {
        REQUIRE(img.data.size() < (1 << 24), std::runtime_error, "Data size stored must be < 16MB");
        REQUIRE(static_cast<uint32_t>(type) <= 255, std::runtime_error, "Type value must be <= 255");
        const uint32_t sizeAndType = ((size & 0xFFFFFF) << 24) & (static_cast<uint32_t>(type) & 0xFF);
        return {img.fileName, img.type, img.size, img.format, prependValue(img.data, size), img.colorMap};
    }

    std::vector<Data> Processing::processBatch(const std::vector<Data> &data, bool clearState)
    {
        REQUIRE(data.size() > 0, std::runtime_error, "Empty data passed to processing");
        std::vector<Data> processed = data;
        for (auto &step : m_steps)
        {
            auto &stepFunc = ProcessingFunctions.find(step.type)->second;
            // we're silently ignoring OperationType::Input operations here
            if (stepFunc.type == OperationType::Convert)
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc.func);
                for (auto &img : processed)
                {
                    const uint32_t inputSize = img.data.size();
                    img = convertFunc(img, step.parameters);
                    if (step.prependProcessing)
                    {
                        img = prependProcessing(img, static_cast<uint32_t>(inputSize), step.type);
                    }
                }
            }
            else if (stepFunc.type == OperationType::ConvertState)
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc.func);
                for (auto &img : processed)
                {
                    const uint32_t inputSize = img.data.size();
                    img = convertFunc(img, step.parameters, step.state);
                    if (step.prependProcessing)
                    {
                        img = prependProcessing(img, static_cast<uint32_t>(inputSize), step.type);
                    }
                }
            }
            else if (stepFunc.type == OperationType::BatchConvert)
            {
                // get all input sizes
                std::vector<uint32_t> inputSizes = {};
                std::transform(processed.cbegin(), processed.cend(), std::back_inserter(inputSizes), [](const auto &d)
                               { return d.data.size(); });
                auto batchFunc = std::get<BatchConvertFunc>(stepFunc.func);
                processed = batchFunc(processed, step.parameters);
                for (auto pIt = processed.begin(); pIt != processed.end(); pIt++)
                {
                    const uint32_t inputSize = inputSizes.at(std::distance(processed.begin(), pIt));
                    *pIt = prependProcessing(*pIt, static_cast<uint32_t>(inputSize), step.type);
                }
            }
            else if (stepFunc.type == OperationType::Reduce)
            {
                auto reduceFunc = std::get<ReduceFunc>(stepFunc.func);
                processed = {reduceFunc(processed, step.parameters)};
            }
        }
        return processed;
    }

    Data Processing::processStream(const Magick::Image &image, bool clearState)
    {
        REQUIRE(ProcessingFunctions.find(m_steps.front().type)->second.type == OperationType::Input, std::runtime_error, "First step must be an input step");
        Data processed;
        for (auto &step : m_steps)
        {
            const uint32_t inputSize = processed.data.size();
            auto &stepFunc = ProcessingFunctions.find(step.type)->second;
            if (stepFunc.type == OperationType::Input)
            {
                auto inputFunc = std::get<InputFunc>(stepFunc.func);
                processed = inputFunc(image, step.parameters);
            }
            else if (stepFunc.type == OperationType::Convert)
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc.func);
                processed = convertFunc(processed, step.parameters);
            }
            else if (stepFunc.type == OperationType::ConvertState)
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc.func);
                processed = convertFunc(processed, step.parameters, step.state);
            }
            // we're silently ignoring OperationType::BatchConvert and ::Reduce operations here
            if (step.prependProcessing)
            {
                processed = prependProcessing(processed, static_cast<uint32_t>(inputSize), step.type);
            }
        }
        return processed;
    }

}
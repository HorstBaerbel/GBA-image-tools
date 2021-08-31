#include "imageprocessing.h"

#include "exception.h"
#include "imagehelpers.h"
#include "colorhelpers.h"
#include "datahelpers.h"
#include "lzsshelpers.h"
#include "spritehelpers.h"

const std::map<ImageProcessing::Type, ImageProcessing::ProcessingFunc>
    ImageProcessing::ProcessingFunctions = {
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
        {Type::PadImageData, {"pad image data", OperationType::Convert, FunctionType(padImageData)}},
        {Type::PadColorMap, {"pad color map", OperationType::Convert, FunctionType(padColorMap)}},
        {Type::EqualizeColorMaps, {"equalize color maps", OperationType::BatchConvert, FunctionType(equalizeColorMaps)}}};

ImageProcessing::Data ImageProcessing::toTiles(const Data &image, const std::vector<Parameter> &parameters)
{
    Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, {}, image.colorMap};
    result.data = convertToTiles(image.data, image.size.width(), image.size.height(), image.bitsPerPixel);
    return result;
}

ImageProcessing::Data ImageProcessing::toSprites(const Data &image, const std::vector<Parameter> &parameters)
{
    // get parameter(s)
    REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "toSprites expects a single uint32_t sprite width parameter");
    auto spriteWidth = std::get<uint32_t>(parameters.front());
    // convert image to sprites
    if (image.size.width() != spriteWidth)
    {
        Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, {}, image.colorMap};
        result.data = convertToWidth(image.data, result.size.width(), result.size.height(), result.bitsPerPixel, spriteWidth);
        result.size = Magick::Geometry(spriteWidth, (result.size.width() * result.size.height()) / spriteWidth);
        return result;
    }
    return image;
}

ImageProcessing::Data ImageProcessing::addColor0(const Data &image, const std::vector<Parameter> &parameters)
{
    // get parameter(s)
    REQUIRE(parameters.size() == 1 && std::holds_alternative<Magick::Color>(parameters.front()), std::runtime_error, "addColor0 expects a single Color parameter");
    auto color0 = std::get<Magick::Color>(parameters.front());
    // checkl of space in color map
    REQUIRE(image.colorMap.size() <= 255, std::runtime_error, "No space in color map (image has " << image.colorMap.size() << " colors)");
    // add color at front of color map
    Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, {}, {}};
    result.colorMap = addColorAtIndex0(image.colorMap, color0);
    result.data = incImageIndicesBy1(image.data);
    return result;
}

ImageProcessing::Data ImageProcessing::moveColor0(const Data &image, const std::vector<Parameter> &parameters)
{
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
        Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, {}, image.colorMap};
        // move index in color map and image data
        std::swap(result.colorMap[oldIndex], result.colorMap[0]);
        result.data = swapIndexToIndex0(image.data, oldIndex);
        return result;
    }
    return image;
}

ImageProcessing::Data ImageProcessing::reorderColors(const Data &image, const std::vector<Parameter> &parameters)
{
    REQUIRE(image.type == Magick::ImageType::PaletteType, std::runtime_error, "Reordering colors can only be done for paletted images");
    Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, {}, {}};
    const auto newOrder = minimizeColorDistance(image.colorMap);
    result.colorMap = swapColors(image.colorMap, newOrder);
    result.data = swapIndices(image.data, newOrder);
    return result;
}

ImageProcessing::Data ImageProcessing::shiftIndices(const Data &image, const std::vector<Parameter> &parameters)
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

ImageProcessing::Data ImageProcessing::pruneIndices(const Data &image, const std::vector<Parameter> &parameters)
{
    REQUIRE(image.type == ImageType::PaletteType, std::runtime_error, "Index pruning only possible paletted images");
    REQUIRE(image.colorMap.size() < 16, std::runtime_error, "Index pruning only possible for images with <= 16 colors");
    uint8_t maxIndex = std::max(*std::max_element(image.data.cbegin(), image.data.cend()), maxIndex);
    REQUIRE(maxIndex < 16, std::runtime_error, "Index pruning only possible for images with <= 16 colors");
    Data result = {image.fileName, image.type, image.size, 4, {}, image.colorMap};
    result.data = convertDataToNibbles(image.data);
    return result;
}

ImageProcessing::Data ImageProcessing::toDelta8(const Data &image, const std::vector<Parameter> &parameters)
{
    Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, {}, image.colorMap};
    result.data = deltaEncode(image.data);
    return result;
}

ImageProcessing::Data ImageProcessing::toDelta16(const Data &image, const std::vector<Parameter> &parameters)
{
    Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, {}, image.colorMap};
    result.data = convertTo<uint8_t>(deltaEncode(convertTo<uint16_t>(image.data)));
    return result;
}

ImageProcessing::Data ImageProcessing::compressLZ10(const Data &image, const std::vector<Parameter> &parameters)
{
    // get parameter(s)
    REQUIRE(parameters.size() == 1 && std::holds_alternative<bool>(parameters.front()), std::runtime_error, "compressLZ10 expects a single bool VRAMcompatible parameter");
    auto vramCompatible = std::get<bool>(parameters.front());
    // compress data
    Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, {}, image.colorMap};
    result.data = Lzss::compressLzss(image.data, vramCompatible, false);
    return result;
}

ImageProcessing::Data ImageProcessing::compressLZ11(const Data &image, const std::vector<Parameter> &parameters)
{
    // get parameter(s)
    REQUIRE(parameters.size() == 1 && std::holds_alternative<bool>(parameters.front()), std::runtime_error, "compressLZ11 expects a single bool VRAMcompatible parameter");
    auto vramCompatible = std::get<bool>(parameters.front());
    // compress data
    Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, {}, image.colorMap};
    result.data = Lzss::compressLzss(image.data, vramCompatible, true);
    return result;
}

ImageProcessing::Data ImageProcessing::padImageData(const Data &image, const std::vector<Parameter> &parameters)
{
    // get parameter(s)
    REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padImageData expects a single uint32_t pad modulo parameter");
    auto multipleOf = std::get<uint32_t>(parameters.front());
    // pad data
    Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, {}, image.colorMap};
    result.data = fillUpToMultipleOf(image.data, multipleOf);
    return result;
}

ImageProcessing::Data ImageProcessing::padColorMap(const Data &image, const std::vector<Parameter> &parameters)
{
    // get parameter(s)
    REQUIRE(parameters.size() == 1 && std::holds_alternative<uint32_t>(parameters.front()), std::runtime_error, "padColorMap expects a single uint32_t pad modulo parameter");
    auto multipleOf = std::get<uint32_t>(parameters.front());
    // pad data
    Data result = {image.fileName, image.type, image.size, image.bitsPerPixel, image.data, {}};
    result.colorMap = fillUpToMultipleOf(image.colorMap, multipleOf);
    return result;
}

std::vector<ImageProcessing::Data> ImageProcessing::equalizeColorMaps(const std::vector<Data> &images, const std::vector<Parameter> &parameters)
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

void ImageProcessing::addStep(Type type)
{
    m_steps.push_back({type, {}});
}

void ImageProcessing::addStep(Type type, const Parameter &parameter)
{
    m_steps.push_back({type, {parameter}});
}

void ImageProcessing::addStep(Type type, const std::vector<Parameter> &parameters)
{
    m_steps.push_back({type, parameters});
}

std::size_t ImageProcessing::size() const
{
    return m_steps.size();
}

void ImageProcessing::clear()
{
    m_steps.clear();
}

std::string ImageProcessing::getProcessingDescription(const std::string &seperator)
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
            result += (pi < (step.parameters.size() - 1) ? " " : "");
        }
        result += (si < (m_steps.size() - 1) ? seperator : "");
    }
    return result;
}

std::vector<ImageProcessing::Data> ImageProcessing::process(const std::vector<Data> &data)
{
    REQUIRE(data.size() > 0, std::runtime_error, "Empty data passed to processing");
    std::vector<Data> processed = data;
    for (const auto &step : m_steps)
    {
        const auto &stepFunc = ProcessingFunctions.find(step.type)->second;
        if (stepFunc.type == OperationType::Convert)
        {
            auto convertFunc = std::get<ConvertType>(stepFunc.func);
            for (auto &img : processed)
            {
                img = convertFunc(img, step.parameters);
            }
        }
        else if (stepFunc.type == OperationType::BatchConvert)
        {
            auto batchFunc = std::get<BatchConvertType>(stepFunc.func);
            processed = batchFunc(processed, step.parameters);
        }
        else if (stepFunc.type == OperationType::Combine)
        {
            auto combineFunc = std::get<CombineType>(stepFunc.func);
            if (processed.size() > 1)
            {
                Data previousImg = processed.front();
                // leave first image verbatim
                for (std::size_t i = 1; i < processed.size(); i++)
                {
                    auto newImg = combineFunc(previousImg, processed[i], step.parameters);
                    // save current image as we're overwriting it now
                    previousImg = processed[i];
                    processed[i] = newImg;
                }
            }
        }
        else if (stepFunc.type == OperationType::Reduce)
        {
            auto reduceFunc = std::get<ReduceType>(stepFunc.func);
            processed = {reduceFunc(processed, step.parameters)};
        }
    }
    return processed;
}

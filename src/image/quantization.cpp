#include "quantization.h"

#include "exception.h"
#include "math/colorfit.h"
#include "math/histogram.h"

namespace Image
{

    auto Quantization::quantizeThreshold(const ImageData &data, float threshold) -> ImageData
    {
        REQUIRE(!data.pixels().empty(), std::runtime_error, "Input data can not be empty");
        REQUIRE(data.pixels().isTruecolor() || data.pixels().isGrayscale(), std::runtime_error, "Input data must be truecolor or grayscale");
        REQUIRE(threshold >= 0.0F && threshold <= 1.0F, std::runtime_error, "Threshold must be in [0.0, 1.0]");
        auto grayscale = data.pixels().convertData<Color::Grayf>();
        std::vector<uint8_t> result;
        std::transform(grayscale.cbegin(), grayscale.cend(), std::back_inserter(result), [threshold](auto v)
                       { return v < threshold ? 0 : 1; });
        return ImageData(result, Color::Format::Paletted8, std::vector<Color::XRGB8888>({0x00000000, 0x00FFFFFF}));
    }

    auto Quantization::quantizeClosest(const ImageData &data, uint32_t nrOfColors, const std::vector<Color::XRGB8888> &colorSpaceMap) -> ImageData
    {
        REQUIRE(!data.pixels().empty(), std::runtime_error, "Input data can not be empty");
        REQUIRE(data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "RGB888 input data expected");
        REQUIRE(nrOfColors >= 1 && nrOfColors <= 255, std::runtime_error, "Number of colors must be in [2, 255]");
        REQUIRE(colorSpaceMap.size() > 0, std::runtime_error, "Color space map can not be empty");
        // use cluster fit to find clusters for colors
        ColorFit<Color::XRGB8888> colorFit(colorSpaceMap);
        const auto srcPixels = data.pixels().data<Color::XRGB8888>();
        auto colorMapping = colorFit.reduceColors(Histogram::buildHistogram(srcPixels), nrOfColors);
        REQUIRE(colorMapping.size() > 0 && nrOfColors >= colorMapping.size(), std::runtime_error, "Unexpected number of mapped colors");
        // build color map
        std::vector<Color::XRGB8888> colorMap;
        std::transform(colorMapping.cbegin(), colorMapping.cend(), std::back_inserter(colorMap), [](const auto &m)
                       { return m.first; });
        // reverse color mapping
        std::map<Color::XRGB8888, uint8_t> reverseMapping;
        std::for_each(colorMapping.cbegin(), colorMapping.cend(), [&reverseMapping, index = 0](const auto &m) mutable
                      { std::transform(m.second.cbegin(), m.second.cend(), std::inserter(reverseMapping, reverseMapping.end()), [index](auto srcColor)
                                      { return std::make_pair(srcColor, index); }); ++index; });
        // map pixel colors to indices
        std::vector<uint8_t> result;
        result.reserve(srcPixels.size());
        std::transform(srcPixels.cbegin(), srcPixels.cend(), std::back_inserter(result), [&reverseMapping](auto srcPixel)
                       { return reverseMapping[srcPixel]; });
        return ImageData(result, Color::Format::Paletted8, colorMap);
    }

    auto Quantization::atkinsonDither(const ImageData &data, uint32_t nrOfColors, const std::vector<Color::XRGB8888> &colorSpaceMap) -> ImageData
    {
        REQUIRE(!data.pixels().empty(), std::runtime_error, "Input data can not be empty");
        REQUIRE(data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "RGB888 input data expected");
        REQUIRE(nrOfColors >= 2 && nrOfColors <= 255, std::runtime_error, "Number of colors must be in [2, 255]");
        REQUIRE(colorSpaceMap.size() > 0, std::runtime_error, "Color space map can not be empty");
        return ImageData{};
    }

}

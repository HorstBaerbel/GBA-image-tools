#include "quantization.h"

#include "exception.h"
#include "math/colorfit.h"

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

    auto Quantization::quantizeClosest(const ImageData &data, const std::map<Color::XRGB8888, std::vector<Color::XRGB8888>> &colorMapping) -> ImageData
    {
        REQUIRE(!data.pixels().empty(), std::runtime_error, "Input data can not be empty");
        REQUIRE(data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "RGB888 input data expected");
        REQUIRE(colorMapping.size() > 0, std::runtime_error, "Color mapping can not be empty");
        // build color map
        std::vector<Color::XRGB8888> resultColorMap;
        std::transform(colorMapping.cbegin(), colorMapping.cend(), std::back_inserter(resultColorMap), [](const auto &m)
                       { return m.first; });
        // reverse color mapping
        std::map<Color::XRGB8888, uint8_t> reverseMapping;
        std::for_each(colorMapping.cbegin(), colorMapping.cend(), [&reverseMapping, index = 0](const auto &m) mutable
                      { std::transform(m.second.cbegin(), m.second.cend(), std::inserter(reverseMapping, reverseMapping.end()), [index](auto srcColor)
                                      { return std::make_pair(srcColor, index); }); ++index; });
        // map pixel colors to indices
        const auto srcPixels = data.pixels().data<Color::XRGB8888>();
        std::vector<uint8_t> resultPixels;
        resultPixels.reserve(srcPixels.size());
        std::transform(srcPixels.cbegin(), srcPixels.cend(), std::back_inserter(resultPixels), [&reverseMapping](auto srcPixel)
                       { return reverseMapping.at(srcPixel); });
        return ImageData(resultPixels, Color::Format::Paletted8, resultColorMap);
    }

    auto Quantization::atkinsonDither(const ImageData &data, uint32_t width, uint32_t height, const std::map<Color::XRGB8888, std::vector<Color::XRGB8888>> &colorMapping) -> ImageData
    {
        REQUIRE(!data.pixels().empty(), std::runtime_error, "Input data can not be empty");
        REQUIRE(data.pixels().format() == Color::Format::XRGB8888, std::runtime_error, "RGB888 input data expected");
        REQUIRE(width > 0 && height > 0, std::runtime_error, "Bad input image size");
        REQUIRE(colorMapping.size() > 0, std::runtime_error, "Color mapping can not be empty");
        return ImageData{};
    }

}

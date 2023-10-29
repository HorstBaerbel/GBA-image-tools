#include "quantization.h"

#include "exception.h"

namespace Image
{

    auto quantizeThreshold(const ImageData &data, float threshold) -> ImageData
    {
        REQUIRE(data.pixels().isTruecolor() || data.pixels().isGrayscale(), "Input data must be truecolor or grayscale");
        REQUIRE(!data.pixels().empty(), "Input data can not be empty");
        auto grayscale = data.pixels().convertData<Color::Grayf>();
        std::vector<uint8_t> result;
        std::transform(grayscale.cbegin(), grayscale.cend(), std::back_inserter(result), [threshold](auto v)
                       { return v.raw() < threshold ? 0 : 1; });
        return ImageData(result, Color::Format::Paletted8, std::vector<Color::XRGB8888>({0x00000000, 0x00FFFFFF}));
    }

    auto quantizeClosest(const ImageData &data, uint32_t nrOfColor, const std::vector<Color::XRGB8888> &colorMap) -> ImageData
    {
        return ImageData{};
    }

    auto atkinsonDither(const ImageData &data, uint32_t nrOfColor, const std::vector<Color::XRGB8888> &colorMap) -> ImageData
    {
        REQUIRE(colorMap.size() > 0, std::runtime_error, "Color map can not be empty");
        return ImageData{};
    }

}

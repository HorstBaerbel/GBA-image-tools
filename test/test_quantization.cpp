#include "testmacros.h"

#include "image/imagehelpers.h"
#include "image/quantization.h"

#include <algorithm>
#include <random>
#include <vector>

using namespace Image;

TEST_SUITE("Quantization")

TEST_CASE("quantizeThreshold")
{
    // XRGB8888 truecolor input: black and white pixels
    // gray = (0.2126*R + 0.7152*G + 0.0722*B) / 255
    std::vector<Color::XRGB8888> pixels{
        Color::XRGB8888(0, 0, 0),       // gray 0.0
        Color::XRGB8888(255, 255, 255), // gray 1.0
        Color::XRGB8888(0, 0, 0),       // gray 0.0
        Color::XRGB8888(255, 255, 255)  // gray 1.0
    };
    ImageData input(pixels);
    auto result = Quantization::quantizeThreshold(input, 0.5F);
    CATCH_REQUIRE(result.pixels().format() == Color::Format::Paletted8);
    CATCH_REQUIRE(result.pixels().size() == 4);
    CATCH_REQUIRE(result.pixels().data<uint8_t>() == std::vector<uint8_t>({0, 1, 0, 1}));
    CATCH_REQUIRE(result.colorMap().size() == 2);
    CATCH_REQUIRE(result.colorMap().data<Color::XRGB8888>() ==
                  std::vector<Color::XRGB8888>({Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255)}));
    // threshold at 0 maps everything to 1 (v < 0 is never true)
    auto allOnes = Quantization::quantizeThreshold(input, 0.0F);
    CATCH_REQUIRE(allOnes.pixels().data<uint8_t>() == std::vector<uint8_t>({1, 1, 1, 1}));
    // threshold at 1 maps everything to 0 except exact 1.0 gray which stays 1
    auto atOne = Quantization::quantizeThreshold(input, 1.0F);
    CATCH_REQUIRE(atOne.pixels().data<uint8_t>() == std::vector<uint8_t>({0, 1, 0, 1}));
}

TEST_CASE("quantizeThresholdValidation")
{
    std::vector<Color::XRGB8888> pixels{Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255)};
    ImageData input(pixels);
    // empty input throws
    ImageData empty;
    CATCH_REQUIRE_THROWS(Quantization::quantizeThreshold(empty, 0.5F));
    // out-of-range thresholds throw
    CATCH_REQUIRE_THROWS(Quantization::quantizeThreshold(input, -0.1F));
    CATCH_REQUIRE_THROWS(Quantization::quantizeThreshold(input, 1.1F));
}

TEST_CASE("quantizeClosest")
{
    // Build a color mapping: target color -> source colors
    std::map<Color::XRGB8888, std::vector<Color::XRGB8888>> colorMapping;
    colorMapping[Color::XRGB8888(0, 0, 0)] = {Color::XRGB8888(0, 0, 0), Color::XRGB8888(10, 10, 10)};
    colorMapping[Color::XRGB8888(255, 255, 255)] = {Color::XRGB8888(255, 255, 255)};
    std::vector<Color::XRGB8888> pixels{
        Color::XRGB8888(0, 0, 0),
        Color::XRGB8888(255, 255, 255),
        Color::XRGB8888(10, 10, 10)};
    ImageData input(pixels);
    auto result = Quantization::quantizeClosest(input, colorMapping);
    CATCH_REQUIRE(result.pixels().format() == Color::Format::Paletted8);
    CATCH_REQUIRE(result.pixels().size() == 3);
    // (0,0,0)->index 0, (255,255,255)->index 1, (10,10,10)->index 0
    CATCH_REQUIRE(result.pixels().data<uint8_t>() == std::vector<uint8_t>({0, 1, 0}));
    CATCH_REQUIRE(result.colorMap().size() == 2);
    CATCH_REQUIRE(result.colorMap().data<Color::XRGB8888>() ==
                  std::vector<Color::XRGB8888>({Color::XRGB8888(0, 0, 0), Color::XRGB8888(255, 255, 255)}));
}

TEST_CASE("quantizeClosestValidation")
{
    std::vector<Color::XRGB8888> pixels{Color::XRGB8888(0, 0, 0)};
    ImageData input(pixels);
    std::map<Color::XRGB8888, std::vector<Color::XRGB8888>> emptyMapping;
    std::map<Color::XRGB8888, std::vector<Color::XRGB8888>> mapping;
    mapping[Color::XRGB8888(0, 0, 0)] = {Color::XRGB8888(0, 0, 0)};
    // empty mapping throws
    CATCH_REQUIRE_THROWS(Quantization::quantizeClosest(input, emptyMapping));
    // empty input throws
    ImageData empty;
    CATCH_REQUIRE_THROWS(Quantization::quantizeClosest(empty, mapping));
}

#include "testmacros.h"

#include "color/psnr.h"
#include "color/rgb888.h"
#include "color/colorhelpers.h"
#include "image/imageio.h"
#include "math/colorfit.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct ColorfitTestFile
{
    std::string fileName;
    float minPsnr16;
    float minPsnr64;
    float minPsnr256;
};

static const std::vector<ColorfitTestFile> ColorfitTestFiles555 = {
    {"artificial_384x256.png", 16.71, 19.66, 21.19},
    {"BigBuckBunny_282_384x256.png", 18.13, 19.70, 21.53},
    {"BigBuckBunny_361_384x256.png", 16.37, 18.63, 20.11},
    {"BigBuckBunny_40_384x256.png", 24.42, 24.64, 24.70},
    {"BigBuckBunny_648_384x256.png", 17.03, 19.25, 20.55},
    {"BigBuckBunny_664_384x256.png", 16.75, 19.08, 20.62},
    {"bridge_256x384.png", 17.32, 19.35, 20.42},
    {"flower_foveon_384x256.png", 17.38, 19.33, 20.67},
    {"gradient_384x256.png", 16.04, 21.08, 24.25},
    {"nightshot_iso_100_384x256.png", 18.21, 20.16, 21.01},
    {"squish_384x384.png", 16.28, 20.47, 22.95},
    {"TearsOfSteel_1200_384x256.png", 16.23, 18.29, 19.93},
    {"TearsOfSteel_676_384x256.png", 16.80, 19.11, 20.77}};

static const std::vector<ColorfitTestFile> ColorfitTestFiles565 = {
    {"artificial_384x256.png", 16.75, 19.51, 21.35},
    {"BigBuckBunny_282_384x256.png", 18.33, 19.80, 21.81},
    {"BigBuckBunny_361_384x256.png", 16.41, 18.78, 20.50},
    {"BigBuckBunny_40_384x256.png", 24.62, 25.35, 25.49},
    {"BigBuckBunny_648_384x256.png", 17.16, 19.39, 20.94},
    {"BigBuckBunny_664_384x256.png", 16.81, 19.18, 21.03},
    {"bridge_256x384.png", 17.39, 19.52, 21.05},
    {"flower_foveon_384x256.png", 17.39, 19.80, 21.31},
    {"gradient_384x256.png", 15.90, 20.85, 24.34},
    {"nightshot_iso_100_384x256.png", 18.31, 20.55, 21.71},
    {"squish_384x384.png", 16.32, 20.50, 23.51},
    {"TearsOfSteel_1200_384x256.png", 16.30, 18.46, 20.22},
    {"TearsOfSteel_676_384x256.png", 16.91, 19.34, 21.29}};

/*
XRGB1555
Quantized artificial_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 16.72, 19.67, 21.2
Quantized BigBuckBunny_282_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 18.14, 19.78, 21.54
Quantized BigBuckBunny_361_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 16.4, 18.64, 20.16
Quantized BigBuckBunny_40_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 24.43, 24.65, 24.71
Quantized BigBuckBunny_648_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 17.04, 19.26, 20.56
Quantized BigBuckBunny_664_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 16.76, 19.09, 20.63
Quantized bridge_256x384.png to RGB555 with 16, 64, 256 colors, psnr: 17.33, 19.36, 20.43
Quantized flower_foveon_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 17.39, 19.34, 20.68
Quantized gradient_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 16.05, 21.08, 24.26
Quantized nightshot_iso_100_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 18.22, 20.17, 21.01
Quantized squish_384x384.png to RGB555 with 16, 64, 256 colors, psnr: 16.29, 20.48, 22.96
Quantized TearsOfSteel_1200_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 16.24, 18.37, 19.94
Quantized TearsOfSteel_676_384x256.png to RGB555 with 16, 64, 256 colors, psnr: 16.81, 19.12, 20.81

RGB565
Quantized artificial_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 16.76, 19.57, 21.36
Quantized BigBuckBunny_282_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 18.34, 20.03, 21.9
Quantized BigBuckBunny_361_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 16.49, 18.83, 20.52
Quantized BigBuckBunny_40_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 24.62, 25.41, 25.49
Quantized BigBuckBunny_648_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 17.16, 19.4, 20.95
Quantized BigBuckBunny_664_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 16.81, 19.27, 21.06
Quantized bridge_256x384.png to RGB565 with 16, 64, 256 colors, psnr: 17.42, 19.53, 21.06
Quantized flower_foveon_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 17.4, 19.83, 21.32
Quantized gradient_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 15.92, 20.86, 24.43
Quantized nightshot_iso_100_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 18.39, 20.59, 21.87
Quantized squish_384x384.png to RGB565 with 16, 64, 256 colors, psnr: 16.33, 20.61, 23.54
Quantized TearsOfSteel_1200_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 16.31, 18.52, 20.23
Quantized TearsOfSteel_676_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 16.94, 19.35, 21.3
*/

static const std::string DataPathTest = "../../data/images/test/";

// #define WRITE_OUTPUT

TEST_SUITE("Colorfit")

template <class PIXEL_TYPE>
auto mapColors(const std::vector<PIXEL_TYPE> &srcPixels, const std::map<PIXEL_TYPE, std::vector<PIXEL_TYPE>> &colorMapping) -> std::vector<PIXEL_TYPE>
{
    // reverse color mapping
    std::map<PIXEL_TYPE, PIXEL_TYPE> reverseMapping;
    std::for_each(colorMapping.cbegin(), colorMapping.cend(), [&reverseMapping](const auto &m) mutable
                  { std::transform(m.second.cbegin(), m.second.cend(), std::inserter(reverseMapping, reverseMapping.end()), [outColor = m.first](auto inColor)
                                   { return std::make_pair(inColor, outColor); }); });
    // map pixel colors to new colors
    std::vector<PIXEL_TYPE> resultPixels;
    resultPixels.reserve(srcPixels.size());
    std::transform(srcPixels.cbegin(), srcPixels.cend(), std::back_inserter(resultPixels), [&reverseMapping](auto srcPixel)
                   { return reverseMapping.at(srcPixel); });
    return resultPixels;
}

TEST_CASE("Colorfit555")
{
    const auto colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::XRGB1555);
    ColorFit<Color::XRGB8888> colorfit(colorSpaceMap);
    for (auto &testFile : ColorfitTestFiles555)
    {
        const auto image = IO::File::readImage(DataPathTest + testFile.fileName);
        const auto inPixels = image.data.pixels().convertData<Color::XRGB8888>();
        const auto inPixelsGamma = Color::srgbToLinear(inPixels);
        const auto outMapping16 = colorfit.reduceColors(inPixels, 16);
        const auto outPixels16 = mapColors(inPixels, outMapping16);
        const auto outPixels16Gamma = Color::srgbToLinear(outPixels16);
        auto psnr16 = Color::psnr(inPixelsGamma, outPixels16Gamma);
        const auto outMapping64 = colorfit.reduceColors(inPixels, 64);
        const auto outPixels64 = mapColors(inPixels, outMapping64);
        const auto outPixels64Gamma = Color::srgbToLinear(outPixels64);
        auto psnr64 = Color::psnr(inPixelsGamma, outPixels64Gamma);
        const auto outMapping256 = colorfit.reduceColors(inPixels, 256);
        const auto outPixels256 = mapColors(inPixels, outMapping256);
        const auto outPixels256Gamma = Color::srgbToLinear(outPixels256);
        auto psnr256 = Color::psnr(inPixelsGamma, outPixels256Gamma);
        std::cout << "Quantized " << testFile.fileName << " to RGB555 with 16, 64, 256 colors, psnr: " << std::setprecision(4) << psnr16 << ", " << psnr64 << ", " << psnr256 << std::endl;
        CATCH_REQUIRE(psnr16 >= testFile.minPsnr16);
        CATCH_REQUIRE(psnr64 >= testFile.minPsnr64);
        CATCH_REQUIRE(psnr256 >= testFile.minPsnr256);
    }
}

TEST_CASE("Colorfit565")
{
    const auto colorSpaceMap = ColorHelpers::buildColorMapFor(Color::Format::RGB565);
    ColorFit<Color::XRGB8888> colorfit(colorSpaceMap);
    for (auto &testFile : ColorfitTestFiles565)
    {
        const auto image = IO::File::readImage(DataPathTest + testFile.fileName);
        const auto inPixels = image.data.pixels().convertData<Color::XRGB8888>();
        const auto inPixelsGamma = Color::srgbToLinear(inPixels);
        const auto outMapping16 = colorfit.reduceColors(inPixels, 16);
        const auto outPixels16 = mapColors(inPixels, outMapping16);
        const auto outPixels16Gamma = Color::srgbToLinear(outPixels16);
        auto psnr16 = Color::psnr(inPixelsGamma, outPixels16Gamma);
        const auto outMapping64 = colorfit.reduceColors(inPixels, 64);
        const auto outPixels64 = mapColors(inPixels, outMapping64);
        const auto outPixels64Gamma = Color::srgbToLinear(outPixels64);
        auto psnr64 = Color::psnr(inPixelsGamma, outPixels64Gamma);
        const auto outMapping256 = colorfit.reduceColors(inPixels, 256);
        const auto outPixels256 = mapColors(inPixels, outMapping256);
        const auto outPixels256Gamma = Color::srgbToLinear(outPixels256);
        auto psnr256 = Color::psnr(inPixelsGamma, outPixels256Gamma);
        std::cout << "Quantized " << testFile.fileName << " to RGB565 with 16, 64, 256 colors, psnr: " << std::setprecision(4) << psnr16 << ", " << psnr64 << ", " << psnr256 << std::endl;
        CATCH_REQUIRE(psnr16 >= testFile.minPsnr16);
        CATCH_REQUIRE(psnr64 >= testFile.minPsnr64);
        CATCH_REQUIRE(psnr256 >= testFile.minPsnr256);
    }
}

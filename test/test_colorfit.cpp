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
    {"artificial_384x256.png", 25.80, 30.54, 34.67},
    {"BigBuckBunny_282_384x256.png", 27.86, 33.06, 36.58},
    {"BigBuckBunny_361_384x256.png", 27.02, 31.27, 34.40},
    {"BigBuckBunny_40_384x256.png", 40.64, 41.65, 42.01},
    {"BigBuckBunny_648_384x256.png", 27.80, 32.00, 34.93},
    {"BigBuckBunny_664_384x256.png", 26.94, 32.00, 35.14},
    {"bridge_256x384.png", 28.25, 32.90, 35.54},
    {"flower_foveon_384x256.png", 28.34, 33.29, 36.53},
    {"gradient_384x256.png", 26.54, 36.05, 41.90},
    {"nightshot_iso_100_384x256.png", 30.38, 34.75, 37.48},
    {"squish_384x384.png", 24.10, 32.28, 37.81},
    {"TearsOfSteel_1200_384x256.png", 26.60, 31.13, 34.35},
    {"TearsOfSteel_676_384x256.png", 27.53, 32.76, 36.42}};

static const std::vector<ColorfitTestFile> ColorfitTestFiles565 = {
    {"artificial_384x256.png", 26.14, 30.66, 35.91},
    {"BigBuckBunny_282_384x256.png", 29.83, 33.82, 36.83},
    {"BigBuckBunny_361_384x256.png", 27.10, 31.52, 34.79},
    {"BigBuckBunny_40_384x256.png", 39.92, 41.91, 42.52},
    {"BigBuckBunny_648_384x256.png", 27.79, 32.13, 35.34},
    {"BigBuckBunny_664_384x256.png", 27.00, 32.00, 35.65},
    {"bridge_256x384.png", 28.41, 33.12, 36.23},
    {"flower_foveon_384x256.png", 28.50, 33.54, 36.90},
    {"gradient_384x256.png", 26.58, 36.92, 43.13},
    {"nightshot_iso_100_384x256.png", 30.41, 35.01, 38.04},
    {"squish_384x384.png", 24.21, 32.44, 38.01},
    {"TearsOfSteel_1200_384x256.png", 26.72, 31.27, 34.80},
    {"TearsOfSteel_676_384x256.png", 27.44, 32.92, 37.01}};

/*
XRGB1555
Quantized artificial_384x256.png to 16, 64, 256 colors, psnr: 25.81, 30.55, 34.68
Quantized BigBuckBunny_282_384x256.png to 16, 64, 256 colors, psnr: 27.87, 33.07, 36.59
Quantized BigBuckBunny_361_384x256.png to 16, 64, 256 colors, psnr: 27.03, 31.28, 34.41
Quantized BigBuckBunny_40_384x256.png to 16, 64, 256 colors, psnr: 40.65, 41.66, 42.02
Quantized BigBuckBunny_648_384x256.png to 16, 64, 256 colors, psnr: 27.81, 32.01, 34.94
Quantized BigBuckBunny_664_384x256.png to 16, 64, 256 colors, psnr: 26.95, 32, 35.15
Quantized bridge_256x384.png to 16, 64, 256 colors, psnr: 28.26, 32.91, 35.55
Quantized flower_foveon_384x256.png to 16, 64, 256 colors, psnr: 28.35, 33.3, 36.54
Quantized gradient_384x256.png to 16, 64, 256 colors, psnr: 26.55, 36.25, 41.98
Quantized nightshot_iso_100_384x256.png to 16, 64, 256 colors, psnr: 30.39, 34.76, 37.49
Quantized squish_384x384.png to 16, 64, 256 colors, psnr: 24.11, 32.29, 37.82
Quantized TearsOfSteel_1200_384x256.png to 16, 64, 256 colors, psnr: 26.61, 31.14, 34.36
Quantized TearsOfSteel_676_384x256.png to 16, 64, 256 colors, psnr: 27.54, 32.77, 36.43

RGB565
Quantized artificial_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 26.15, 30.67, 35.92
Quantized BigBuckBunny_282_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 29.84, 33.83, 36.84
Quantized BigBuckBunny_361_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 27.11, 31.53, 34.8
Quantized BigBuckBunny_40_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 39.93, 41.92, 42.53
Quantized BigBuckBunny_648_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 27.8, 32.14, 35.35
Quantized BigBuckBunny_664_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 27.01, 32.01, 35.66
Quantized bridge_256x384.png to RGB565 with 16, 64, 256 colors, psnr: 28.42, 33.13, 36.24
Quantized flower_foveon_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 28.51, 33.55, 36.91
Quantized gradient_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 26.59, 36.96, 43.32
Quantized nightshot_iso_100_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 30.42, 35.02, 38.05
Quantized squish_384x384.png to RGB565 with 16, 64, 256 colors, psnr: 24.22, 32.45, 38.02
Quantized TearsOfSteel_1200_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 26.73, 31.28, 34.81
Quantized TearsOfSteel_676_384x256.png to RGB565 with 16, 64, 256 colors, psnr: 27.45, 32.93, 37.02
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
        const auto outMapping16 = colorfit.reduceColors(inPixels, 16);
        const auto outPixels16 = mapColors(inPixels, outMapping16);
        auto psnr16 = Color::psnr(inPixels, outPixels16);
        const auto outMapping64 = colorfit.reduceColors(inPixels, 64);
        const auto outPixels64 = mapColors(inPixels, outMapping64);
        auto psnr64 = Color::psnr(inPixels, outPixels64);
        const auto outMapping256 = colorfit.reduceColors(inPixels, 256);
        const auto outPixels256 = mapColors(inPixels, outMapping256);
        auto psnr256 = Color::psnr(inPixels, outPixels256);
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
        const auto outMapping16 = colorfit.reduceColors(inPixels, 16);
        const auto outPixels16 = mapColors(inPixels, outMapping16);
        auto psnr16 = Color::psnr(inPixels, outPixels16);
        const auto outMapping64 = colorfit.reduceColors(inPixels, 64);
        const auto outPixels64 = mapColors(inPixels, outMapping64);
        auto psnr64 = Color::psnr(inPixels, outPixels64);
        const auto outMapping256 = colorfit.reduceColors(inPixels, 256);
        const auto outPixels256 = mapColors(inPixels, outMapping256);
        auto psnr256 = Color::psnr(inPixels, outPixels256);
        std::cout << "Quantized " << testFile.fileName << " to RGB565 with 16, 64, 256 colors, psnr: " << std::setprecision(4) << psnr16 << ", " << psnr64 << ", " << psnr256 << std::endl;
        CATCH_REQUIRE(psnr16 >= testFile.minPsnr16);
        CATCH_REQUIRE(psnr64 >= testFile.minPsnr64);
        CATCH_REQUIRE(psnr256 >= testFile.minPsnr256);
    }
}

#include "testmacros.h"

#include "codec/dxt.h"
#include "color/psnr.h"
#include "color/rgb888.h"
#include "io/imageio.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct TestFile
{
    std::string fileName;
    float minPsnr555;
    float minPsnr565;
};

const std::vector<TestFile> TestFiles = {
    {"artificial_384x256.png", 33.27F, 33.50F},
    {"BigBuckBunny_282_384x256.png", 34.89F, 35.27F},
    {"BigBuckBunny_361_384x256.png", 31.65F, 31.84F},
    {"BigBuckBunny_40_384x256.png", 39.40F, 39.73F},
    {"BigBuckBunny_648_384x256.png", 32.55F, 32.72F},
    {"BigBuckBunny_664_384x256.png", 35.48F, 35.97F},
    {"bridge_256x384.png", 31.78F, 31.98F},
    {"flower_foveon_384x256.png", 36.57F, 37.04F},
    {"nightshot_iso_100_384x256.png", 34.70F, 35.08F},
    {"squish_384x384.png", 40.14F, 41.36F},
    {"TearsOfSteel_1200_384x256.png", 33.43F, 33.70F},
    {"TearsOfSteel_676_384x256.png", 34.03F, 34.34F}};

/*
XRGB1555
artificial_384x256.png, psnr: 33.28
BigBuckBunny_282_384x256.png, psnr: 34.9
BigBuckBunny_361_384x256.png, psnr: 31.66
BigBuckBunny_40_384x256.png, psnr: 39.41
BigBuckBunny_648_384x256.png, psnr: 32.56
BigBuckBunny_664_384x256.png, psnr: 35.49
bridge_256x384.png, psnr: 31.79
flower_foveon_384x256.png, psnr: 36.58
nightshot_iso_100_384x256.png, psnr: 34.71
squish_384x384.png, psnr: 40.15
TearsOfSteel_1200_384x256.png, psnr: 33.44
TearsOfSteel_676_384x256.png, psnr: 34.04

RGB565
artificial_384x256.png, psnr: 33.51
BigBuckBunny_282_384x256.png, psnr: 35.28
BigBuckBunny_361_384x256.png, psnr: 31.85
BigBuckBunny_40_384x256.png, psnr: 39.74
BigBuckBunny_648_384x256.png, psnr: 32.73
BigBuckBunny_664_384x256.png, psnr: 35.98
bridge_256x384.png, psnr: 31.99
flower_foveon_384x256.png, psnr: 37.05
nightshot_iso_100_384x256.png, psnr: 35.09
squish_384x384.png, psnr: 41.37
TearsOfSteel_1200_384x256.png, psnr: 33.71
TearsOfSteel_676_384x256.png, psnr: 34.35
*/

const std::string DataPath = "../../data/images/test/";

// #define WRITE_OUTPUT

TEST_SUITE("DXT")

TEST_CASE("EncodeDecode555")
{
    for (auto &testFile : TestFiles)
    {
        auto image = IO::File::readImage(DataPath + testFile.fileName);
        auto inPixels = image.imageData.pixels().convertData<Color::XRGB8888>();
        // IO::File::writeImage(image, "/tmp", "in.png");
        auto compressedData = DXT::encodeDXT(inPixels, image.size.width(), image.size.height(), false);
        auto outPixels = DXT::decodeDXT(compressedData, image.size.width(), image.size.height(), false);
        auto psnr = Color::psnr(inPixels, outPixels);
#ifdef WRITE_OUTPUT
        auto xrgb8888 = Image::PixelData(outPixels, Color::Format::XRGB8888);
        image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
        IO::File::writeImage(image, "/tmp/test555", testFile.fileName);
#endif
        std::cout << "DXT-compressed " << testFile.fileName << ", psnr: " << std::setprecision(4) << psnr << std::endl;
        CATCH_REQUIRE(psnr >= testFile.minPsnr555);
    }
}

TEST_CASE("EncodeDecode565")
{
    for (auto &testFile : TestFiles)
    {
        auto image = IO::File::readImage(DataPath + testFile.fileName);
        auto inPixels = image.imageData.pixels().convertData<Color::XRGB8888>();
        // IO::File::writeImage(image, "/tmp", "in.png");
        auto compressedData = DXT::encodeDXT(inPixels, image.size.width(), image.size.height(), true);
        auto outPixels = DXT::decodeDXT(compressedData, image.size.width(), image.size.height(), true);
        auto psnr = Color::psnr(inPixels, outPixels);
#ifdef WRITE_OUTPUT
        auto xrgb8888 = Image::PixelData(outPixels, Color::Format::XRGB8888);
        image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
        IO::File::writeImage(image, "/tmp/test565", testFile.fileName);
#endif
        std::cout << "DXT-compressed " << testFile.fileName << ", psnr: " << std::setprecision(4) << psnr << std::endl;
        CATCH_REQUIRE(psnr >= testFile.minPsnr565);
    }
}

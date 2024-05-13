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
    {"artificial_384x256.png", 31.85F, 32.01F},
    {"BigBuckBunny_282_384x256.png", 33.68F, 33.96F},
    {"BigBuckBunny_361_384x256.png", 30.01F, 30.11},
    {"BigBuckBunny_40_384x256.png", 38.74F, 39.01F},
    {"BigBuckBunny_648_384x256.png", 31.12F, 31.21F},
    {"BigBuckBunny_664_384x256.png", 34.1F, 34.45F},
    {"bridge_256x384.png", 30.24F, 30.36F},
    {"flower_foveon_384x256.png", 34.98F, 35.3F},
    {"nightshot_iso_100_384x256.png", 33.21F, 33.46F},
    {"squish_384x384.png", 39.8F, 40.79F},
    {"TearsOfSteel_1200_384x256.png", 31.96F, 32.16F},
    {"TearsOfSteel_676_384x256.png", 32.52F, 32.73F}};

/*
XRGB1555
artificial_384x256.png, psnr: 31.85
BigBuckBunny_282_384x256.png, psnr: 33.68
BigBuckBunny_361_384x256.png, psnr: 30.01
BigBuckBunny_40_384x256.png, psnr: 38.74
BigBuckBunny_648_384x256.png, psnr: 31.12
BigBuckBunny_664_384x256.png, psnr: 34.1
bridge_256x384.png, psnr: 30.24
flower_foveon_384x256.png, psnr: 34.98
nightshot_iso_100_384x256.png, psnr: 33.21
squish_384x384.png, psnr: 39.8
TearsOfSteel_1200_384x256.png, psnr: 31.96
TearsOfSteel_676_384x256.png, psnr: 32.52

RGB565
artificial_384x256.png, psnr: 32.01
BigBuckBunny_282_384x256.png, psnr: 33.96
BigBuckBunny_361_384x256.png, psnr: 30.11
BigBuckBunny_40_384x256.png, psnr: 39.01
BigBuckBunny_648_384x256.png, psnr: 31.21
BigBuckBunny_664_384x256.png, psnr: 34.45
bridge_256x384.png, psnr: 30.36
flower_foveon_384x256.png, psnr: 35.3
nightshot_iso_100_384x256.png, psnr: 33.46
squish_384x384.png, psnr: 40.79
TearsOfSteel_1200_384x256.png, psnr: 32.16
TearsOfSteel_676_384x256.png, psnr: 32.73
*/

const std::string DataPath = "../../data/images/test/";

#define WRITE_OUTPUT

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

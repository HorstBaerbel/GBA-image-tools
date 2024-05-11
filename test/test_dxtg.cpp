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
    {"artificial_384x256.png", 31.76F, 31.94F},
    {"BigBuckBunny_282_384x256.png", 33.54F, 33.87F},
    {"BigBuckBunny_361_384x256.png", 29.84F, 30.01},
    {"BigBuckBunny_40_384x256.png", 38.71F, 38.98F},
    {"BigBuckBunny_648_384x256.png", 30.90F, 31.09F},
    {"BigBuckBunny_664_384x256.png", 33.87F, 34.30F},
    {"bridge_256x384.png", 30.08F, 30.24F},
    {"flower_foveon_384x256.png", 34.66F, 35.04F},
    {"nightshot_iso_100_384x256.png", 33.02F, 33.33F},
    {"squish_384x384.png", 38.76F, 40.07F},
    {"TearsOfSteel_1200_384x256.png", 31.75F, 32.03F},
    {"TearsOfSteel_676_384x256.png", 32.28F, 32.60F}};

/*
XRGB1555
artificial_384x256.png, psnr: 31.76
BigBuckBunny_282_384x256.png, psnr: 33.54
BigBuckBunny_361_384x256.png, psnr: 29.84
BigBuckBunny_40_384x256.png, psnr: 38.72
BigBuckBunny_648_384x256.png, psnr: 30.9
BigBuckBunny_664_384x256.png, psnr: 33.88
bridge_256x384.png, psnr: 30.08
flower_foveon_384x256.png, psnr: 34.66
nightshot_iso_100_384x256.png, psnr: 33.02
squish_384x384.png, psnr: 38.77
TearsOfSteel_1200_384x256.png, psnr: 31.75
TearsOfSteel_676_384x256.png, psnr: 32.29

RGB565
artificial_384x256.png, psnr: 31.95
BigBuckBunny_282_384x256.png, psnr: 33.87
BigBuckBunny_361_384x256.png, psnr: 30.01
BigBuckBunny_40_384x256.png, psnr: 38.98
BigBuckBunny_648_384x256.png, psnr: 31.09
BigBuckBunny_664_384x256.png, psnr: 34.3
bridge_256x384.png, psnr: 30.25
flower_foveon_384x256.png, psnr: 35.04
nightshot_iso_100_384x256.png, psnr: 33.33
squish_384x384.png, psnr: 40.08
TearsOfSteel_1200_384x256.png, psnr: 32.03
TearsOfSteel_676_384x256.png, psnr: 32.6
*/

const std::string DataPath = "../../data/images/test/";

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
        // auto xrgb8888 = Image::PixelData(outPixels, Color::Format::XRGB8888);
        // image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
        //  IO::File::writeImage(image, "/tmp/test555", testFile.fileName);
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
        // auto xrgb8888 = Image::PixelData(outPixels, Color::Format::XRGB8888);
        // image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
        //  IO::File::writeImage(image, "/tmp/test565", testFile.fileName);
        std::cout << "DXT-compressed " << testFile.fileName << ", psnr: " << std::setprecision(4) << psnr << std::endl;
        CATCH_REQUIRE(psnr >= testFile.minPsnr565);
    }
}

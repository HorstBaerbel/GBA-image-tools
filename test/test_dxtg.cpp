#include "testmacros.h"

#include "codec/dxt.h"
#include "color/distance.h"
#include "color/rgb888.h"
#include "io/imageio.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct TestFile
{
    std::string fileName;
    float maximumError555;
    float maximumError565;
};

const std::vector<TestFile> TestFiles = {
    {"artificial_384x256.png", 0.0007F, 0.0007F},
    {"BigBuckBunny_282_384x256.png", 0.0005F, 0.0005F},
    {"BigBuckBunny_361_384x256.png", 0.0011F, 0.001F},
    {"BigBuckBunny_40_384x256.png", 0.0002F, 0.0002F},
    {"BigBuckBunny_648_384x256.png", 0.0009F, 0.0008F},
    {"BigBuckBunny_664_384x256.png", 0.0005F, 0.0004F},
    {"bridge_256x384.png", 0.001F, 0.001F},
    {"flower_foveon_384x256.png", 0.0004F, 0.0004F},
    {"nightshot_iso_100_384x256.png", 0.0005F, 0.0005F},
    {"squish_384x384.png", 0.0002F, 0.0001F},
    {"TearsOfSteel_1200_384x256.png", 0.0007F, 0.0007F},
    {"TearsOfSteel_676_384x256.png", 0.0006F, 0.0006F}};

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
        auto imageError = Color::distance(inPixels, outPixels);
        // auto xrgb8888 = Image::PixelData(outPixels, Color::Format::XRGB8888);
        // image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
        //  IO::File::writeImage(image, "/tmp/test555", testFile.fileName);
        std::cout << "DXT-compressed " << testFile.fileName << ", error: " << std::setprecision(5) << imageError << std::endl;
        CATCH_REQUIRE(imageError < testFile.maximumError555);
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
        auto imageError = Color::distance(inPixels, outPixels);
        // auto xrgb8888 = Image::PixelData(outPixels, Color::Format::XRGB8888);
        // image.imageData.pixels() = Image::PixelData(xrgb8888.convertData<Color::RGB888>(), Color::Format::RGB888);
        //  IO::File::writeImage(image, "/tmp/test565", testFile.fileName);
        std::cout << "DXT-compressed " << testFile.fileName << ", error: " << std::setprecision(5) << imageError << std::endl;
        CATCH_REQUIRE(imageError < testFile.maximumError565);
    }
}

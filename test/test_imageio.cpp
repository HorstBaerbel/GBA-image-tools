#include "testmacros.h"

#include "io/imageio.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

struct IoTestFile
{
    std::string filePath;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t hash = 0;
};

static const std::vector<IoTestFile> IoTestFiles = {
    {"240x160/artificial_240x160.png", 240, 160, 0xada42e2a},
    {"test/BigBuckBunny_282_384x256.png", 384, 256, 0xd3b33f64}};

static const std::string DataPathTest = "../../data/images/";

// MurmurOAAT 32-bit hash function, see: https://stackoverflow.com/a/77342581/1121150
uint32_t Hash_MurmurOAAT_32(const uint8_t *data, size_t size)
{
    // One-byte-at-a-time hash based on Murmur's mix
    uint32_t h = 0x12345678;
    for (size_t i = 0; i < size; i++)
    {
        h ^= data[i];
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

TEST_SUITE("Image I/O")

TEST_CASE("Read + Write")
{
    for (const auto &testFile : IoTestFiles)
    {
        // Read image
        auto inImage = IO::File::readImage(DataPathTest + testFile.filePath);
        CATCH_REQUIRE(inImage.index == 0);
        CATCH_REQUIRE(inImage.fileName == "");
        CATCH_REQUIRE(inImage.type.isBitmap());
        CATCH_REQUIRE(!inImage.type.isCompressed());
        CATCH_REQUIRE(!inImage.type.isSprites());
        CATCH_REQUIRE(!inImage.type.isTiles());
        CATCH_REQUIRE(inImage.map.data.empty());
        CATCH_REQUIRE(inImage.map.size.width() == 0);
        CATCH_REQUIRE(inImage.map.size.height() == 0);
        CATCH_REQUIRE(inImage.image.size.width() == testFile.width);
        CATCH_REQUIRE(inImage.image.size.height() == testFile.height);
        CATCH_REQUIRE(inImage.image.colorMapFormat == Color::Format::Unknown);
        CATCH_REQUIRE(inImage.image.nrOfColorMapEntries == 0);
        CATCH_REQUIRE(inImage.image.pixelFormat == Color::Format::XRGB8888);
        CATCH_REQUIRE(inImage.image.maxMemoryNeeded == 0);
        CATCH_REQUIRE(inImage.image.data.colorMap().empty());
        const auto inPixels = inImage.image.data.pixels().convertData<Color::XRGB8888>();
        CATCH_REQUIRE(inPixels.size() == testFile.width * testFile.height);
        // check hash of pixel data
        const uint32_t inHash = Hash_MurmurOAAT_32(reinterpret_cast<const uint8_t *>(inPixels.data()), inPixels.size() * sizeof(Color::XRGB8888));
        CATCH_REQUIRE(inHash == testFile.hash);
        // write image to temporary file
        std::filesystem::path tempFilePath = std::filesystem::temp_directory_path() / (std::to_string(inHash) + ".png");
        IO::File::writeImage(inImage, "", tempFilePath);
        // read the file again
        auto outImage = IO::File::readImage(tempFilePath);
        CATCH_REQUIRE(inImage.index == outImage.index);
        CATCH_REQUIRE(inImage.fileName == outImage.fileName);
        CATCH_REQUIRE(inImage.type.isBitmap() == outImage.type.isBitmap());
        CATCH_REQUIRE(inImage.type.isCompressed() == outImage.type.isCompressed());
        CATCH_REQUIRE(inImage.type.isSprites() == outImage.type.isSprites());
        CATCH_REQUIRE(inImage.type.isTiles() == outImage.type.isTiles());
        CATCH_REQUIRE(inImage.map.data.empty() == outImage.map.data.empty());
        CATCH_REQUIRE(inImage.map.size.width() == outImage.map.size.width());
        CATCH_REQUIRE(inImage.map.size.height() == outImage.map.size.height());
        CATCH_REQUIRE(inImage.image.size.width() == outImage.image.size.width());
        CATCH_REQUIRE(inImage.image.size.height() == outImage.image.size.height());
        CATCH_REQUIRE(inImage.image.colorMapFormat == outImage.image.colorMapFormat);
        CATCH_REQUIRE(inImage.image.nrOfColorMapEntries == outImage.image.nrOfColorMapEntries);
        CATCH_REQUIRE(inImage.image.pixelFormat == outImage.image.pixelFormat);
        CATCH_REQUIRE(inImage.image.maxMemoryNeeded == outImage.image.maxMemoryNeeded);
        CATCH_REQUIRE(inImage.image.data.colorMap().empty() == outImage.image.data.colorMap().empty());
        const auto outPixels = outImage.image.data.pixels().convertData<Color::XRGB8888>();
        CATCH_REQUIRE(outPixels.size() == testFile.width * testFile.height);
        // check hash of pixel data
        const uint32_t outHash = Hash_MurmurOAAT_32(reinterpret_cast<const uint8_t *>(outPixels.data()), outPixels.size() * sizeof(Color::XRGB8888));
        CATCH_REQUIRE(inHash == outHash);
        // delete temporary file
        std::filesystem::remove(tempFilePath);
    }
}

#include "testmacros.h"

#include "io/binio.h"

#include <cstdio>
#include <fstream>

using namespace IO;

TEST_SUITE("BinIO")

TEST_CASE("writeData uint8_t vector")
{
    const std::string fileName = "/tmp/test_binio_uint8.bin";
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0xFF};
    Bin::writeData(fileName, data);
    // Verify file was written correctly
    std::ifstream file(fileName, std::ios::binary);
    CATCH_REQUIRE(file.is_open());
    std::vector<uint8_t> readData(data.size());
    file.read(reinterpret_cast<char *>(readData.data()), data.size());
    CATCH_REQUIRE(readData == data);
    // Cleanup
    std::remove(fileName.c_str());
}

TEST_CASE("writeData uint32_t vector")
{
    const std::string fileName = "/tmp/test_binio_uint32.bin";
    std::vector<uint32_t> data = {0x12345678, 0xDEADBEEF, 0x00000001};
    Bin::writeData(fileName, data);
    // Verify file was written correctly (4 bytes per element)
    std::ifstream file(fileName, std::ios::binary);
    CATCH_REQUIRE(file.is_open());
    std::vector<uint32_t> readData(data.size());
    file.read(reinterpret_cast<char *>(readData.data()), data.size() * 4);
    CATCH_REQUIRE(readData == data);
    // Cleanup
    std::remove(fileName.c_str());
}

TEST_CASE("writeData empty vector")
{
    const std::string fileName = "/tmp/test_binio_empty.bin";
    std::vector<uint8_t> data;
    Bin::writeData(fileName, data);
    // File should exist but be empty
    std::ifstream file(fileName, std::ios::binary);
    CATCH_REQUIRE(file.is_open());
    CATCH_REQUIRE(file.peek() == std::ifstream::traits_type::eof());
    // Cleanup
    std::remove(fileName.c_str());
}

TEST_CASE("writeData large vector")
{
    const std::string fileName = "/tmp/test_binio_large.bin";
    std::vector<uint8_t> data(10000);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    Bin::writeData(fileName, data);
    std::ifstream file(fileName, std::ios::binary);
    CATCH_REQUIRE(file.is_open());
    std::vector<uint8_t> readData(data.size());
    file.read(reinterpret_cast<char *>(readData.data()), data.size());
    CATCH_REQUIRE(readData == data);
    // Cleanup
    std::remove(fileName.c_str());
}

TEST_CASE("writeData to invalid path throws")
{
    std::vector<uint8_t> data = {1, 2, 3};
    CATCH_REQUIRE_THROWS_AS(
        Bin::writeData("/nonexistent/path/file.bin", data),
        std::runtime_error);
}

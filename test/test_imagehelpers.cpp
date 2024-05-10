#include "testmacros.h"

#include "processing/imagehelpers.h"

#include <algorithm>
#include <vector>

using namespace ImageHelpers;

TEST_SUITE("Image helpers")

TEST_CASE("convertDataTo1Bit")
{
    std::vector<uint8_t> v0;
    v0 = convertDataTo1Bit(v0);
    CATCH_REQUIRE(v0.size() == 0);
    v0.resize(1);
    CATCH_REQUIRE_THROWS(convertDataTo1Bit(v0));
    v0.resize(2);
    CATCH_REQUIRE_THROWS(convertDataTo1Bit(v0));
    v0.resize(3);
    CATCH_REQUIRE_THROWS(convertDataTo1Bit(v0));
    v0.resize(4);
    CATCH_REQUIRE_THROWS(convertDataTo1Bit(v0));
    v0.resize(5);
    CATCH_REQUIRE_THROWS(convertDataTo1Bit(v0));
    v0.resize(6);
    CATCH_REQUIRE_THROWS(convertDataTo1Bit(v0));
    v0.resize(7);
    CATCH_REQUIRE_THROWS(convertDataTo1Bit(v0));
    v0.resize(9);
    CATCH_REQUIRE_THROWS(convertDataTo1Bit(v0));
    std::vector<uint8_t> v1 = {0x01, 0x00, 0x02, 0x01, 0x15, 0x00, 0x01, 0x01};
    CATCH_REQUIRE_THROWS(convertDataTo1Bit(v1));
    std::vector<uint8_t> v2 = {0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01};
    v2 = convertDataTo1Bit(v2);
    CATCH_REQUIRE(v2 == std::vector<uint8_t>{0xDD});
    std::vector<uint8_t> v3 = {0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00};
    v3 = convertDataTo1Bit(v3);
    CATCH_REQUIRE(v3 == std::vector<uint8_t>{0xDD, 0x29});
}

TEST_CASE("convertDataTo2Bit")
{
    std::vector<uint8_t> v0;
    v0 = convertDataTo2Bit(v0);
    CATCH_REQUIRE(v0.size() == 0);
    v0.resize(1);
    CATCH_REQUIRE_THROWS(convertDataTo2Bit(v0));
    v0.resize(2);
    CATCH_REQUIRE_THROWS(convertDataTo2Bit(v0));
    v0.resize(3);
    CATCH_REQUIRE_THROWS(convertDataTo2Bit(v0));
    v0.resize(5);
    CATCH_REQUIRE_THROWS(convertDataTo2Bit(v0));
    std::vector<uint8_t> v1 = {0x01, 0x00, 0x04, 0x01, 0x17, 0x00, 0x01, 0x01};
    CATCH_REQUIRE_THROWS(convertDataTo2Bit(v1));
    std::vector<uint8_t> v2 = {0x01, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00, 0x01};
    v2 = convertDataTo2Bit(v2);
    CATCH_REQUIRE(v2 == std::vector<uint8_t>{0x61, 0x43});
    std::vector<uint8_t> v3 = {0x00, 0x01, 0x02, 0x03, 0x01, 0x02, 0x00, 0x01, 0x03, 0x02, 0x01, 0x00};
    v3 = convertDataTo2Bit(v3);
    CATCH_REQUIRE(v3 == std::vector<uint8_t>{0xE4, 0x49, 0x1B});
}

TEST_CASE("convertDataTo4Bit")
{
    std::vector<uint8_t> v0;
    v0 = convertDataTo4Bit(v0);
    CATCH_REQUIRE(v0.size() == 0);
    v0.resize(1);
    CATCH_REQUIRE_THROWS(convertDataTo4Bit(v0));
    v0.resize(3);
    CATCH_REQUIRE_THROWS(convertDataTo4Bit(v0));
    std::vector<uint8_t> v1 = {0x01, 0x00, 0x04, 0x01, 0x17, 0x00, 0x01, 0x01};
    CATCH_REQUIRE_THROWS(convertDataTo4Bit(v1));
    std::vector<uint8_t> v2 = {0x01, 0x07, 0x0A, 0x03};
    v2 = convertDataTo4Bit(v2);
    CATCH_REQUIRE(v2 == std::vector<uint8_t>{0x71, 0x3A});
    std::vector<uint8_t> v3 = {0x00, 0x0F, 0x03, 0x07, 0x0B, 0x0A, 0x04, 0x00};
    v3 = convertDataTo4Bit(v3);
    CATCH_REQUIRE(v3 == std::vector<uint8_t>{0xF0, 0x73, 0xAB, 0x04});
}

TEST_CASE("incValuesBy1")
{
    std::vector<uint8_t> v0;
    v0 = incValuesBy1(v0);
    CATCH_REQUIRE(v0.size() == 0);
    std::vector<uint8_t> v1 = {0x01, 0x00, 0xFF, 0x01, 0x17, 0x00, 0x01, 0x01};
    CATCH_REQUIRE_THROWS(incValuesBy1(v1));
    std::vector<uint8_t> v2 = {0x01, 0xFE, 0x0A, 0x13, 0x00};
    v2 = incValuesBy1(v2);
    CATCH_REQUIRE(v2 == std::vector<uint8_t>{0x02, 0xFF, 0x0B, 0x14, 0x01});
}

TEST_CASE("swapValueWith0")
{
    std::vector<uint8_t> v0;
    v0 = swapValueWith0(v0, 5);
    CATCH_REQUIRE(v0.size() == 0);
    std::vector<uint8_t> v1 = {0x01, 0x00, 0xFF, 0x01, 0x17, 0x00, 0x01, 0x01, 0xFF};
    v1 = swapValueWith0(v1, 0xFF);
    CATCH_REQUIRE(v1 == std::vector<uint8_t>{0x01, 0xFF, 0x00, 0x01, 0x017, 0xFF, 0x01, 0x01, 0x00});
    std::vector<uint8_t> v2 = {0x01, 0xFE, 0x0A, 0x13, 0x02};
    v2 = swapValueWith0(v2, 0x04);
    CATCH_REQUIRE(v2 == std::vector<uint8_t>{0x01, 0xFE, 0x0A, 0x13, 0x02});
}

TEST_CASE("swapValues")
{
    std::vector<uint8_t> v0;
    v0 = swapValues(v0, {0x00});
    CATCH_REQUIRE(v0.size() == 0);
    v0.resize(1);
    v0 = swapValues(v0, {});
    CATCH_REQUIRE(v0.size() == 1);
    std::vector<uint8_t> v1 = {0x01, 0x03, 0x02, 0x00, 0x5};
    CATCH_REQUIRE_THROWS(swapValues(v1, {0x00}));
    CATCH_REQUIRE_THROWS(swapValues(v1, {0x00, 0x01, 0x02, 0x03, 0x04}));
    v1 = swapValues(v1, {0x11, 0x12, 0x13, 0x14, 0x15, 0x16});
    CATCH_REQUIRE(v1 == std::vector<uint8_t>{0x12, 0x14, 0x13, 0x11, 0x016});
}

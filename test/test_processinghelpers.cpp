#include "testmacros.h"

#include "image/imagedata.h"
#include "image/imagedatahelpers.h"

#include <algorithm>
#include <random>
#include <vector>

using namespace Image;

TEST_SUITE("Processing helpers")

template <typename T>
constexpr auto createData(std::vector<T> &&pixelData, MapData &&mapData) -> Frame
{
    return {0, "", DataType(), {{0, 0}, Color::Format::Unknown, Color::Format::Unknown, 0, 0}, pixelData, {{0, 0}, {mapData}}};
}

template <typename T, typename R>
constexpr auto createImageData(std::vector<T> &&pixels, Color::Format format, std::vector<R> &&colormap) -> Frame
{
    return {0, "", DataType(), {{0, 0}, Color::Format::Unknown, Color::Format::Unknown, 0, 0}, ImageData{pixels, format, colormap}, {{0, 0}, {}}};
}

constexpr auto createMapData(MapData &&mapData) -> Frame
{
    return {0, "", DataType(), {{0, 0}, Color::Format::Unknown, Color::Format::Unknown, 0, 0}, {}, {{0, 0}, {mapData}}};
}

TEST_CASE("combineRawPixelData")
{
    std::vector<Frame> v0;
    auto r0 = combineRawPixelData<uint8_t>(v0);
    CATCH_REQUIRE(r0.first.empty());
    CATCH_REQUIRE(r0.second.empty());
    Frame d0 = createData(std::vector<Color::XRGB8888>({0x00112233, 0x00445566}), {});
    Frame d1 = createData(std::vector<Color::XRGB8888>({0x00778899, 0x00AABBCC}), {});
    std::vector<Frame> v1 = {d0, d1};
    auto r1 = combineRawPixelData<uint8_t>(v1);
    CATCH_REQUIRE(r1.first.size() == 16);
    CATCH_REQUIRE(r1.first == std::vector<uint8_t>({0x33, 0x22, 0x11, 0x00, 0x66, 0x55, 0x44, 0x00, 0x99, 0x88, 0x77, 0x00, 0xCC, 0xBB, 0xAA, 0x00}));
    CATCH_REQUIRE(r1.second.size() == 2);
    CATCH_REQUIRE(r1.second == std::vector<uint32_t>({0, 8}));
    auto r2 = combineRawPixelData<uint16_t>(v1);
    CATCH_REQUIRE(r2.first.size() == 8);
    CATCH_REQUIRE(r2.first == std::vector<uint16_t>({0x2233, 0x0011, 0x5566, 0x0044, 0x8899, 0x0077, 0xBBCC, 0x00AA}));
    CATCH_REQUIRE(r2.second.size() == 2);
    CATCH_REQUIRE(r2.second == std::vector<uint32_t>({0, 4}));
    auto r3 = combineRawPixelData<uint32_t>(v1);
    CATCH_REQUIRE(r3.first.size() == 4);
    CATCH_REQUIRE(r3.first == std::vector<uint32_t>({0x00112233, 0x00445566, 0x00778899, 0x00AABBCC}));
    CATCH_REQUIRE(r3.second.size() == 2);
    CATCH_REQUIRE(r3.second == std::vector<uint32_t>({0, 2}));
    Frame d2 = createData(std::vector<Color::XRGB8888>({0x00112233}), {});
    Frame d3 = createData(std::vector<Color::XRGB8888>({0x00AABBCC, 0x00DDEEFF}), {});
    std::vector<Frame> v2 = {d2, d3};
    CATCH_REQUIRE_THROWS(combineRawPixelData<uint32_t>(v2, true));
    auto r4 = combineRawPixelData<uint32_t>(v2);
    CATCH_REQUIRE(r4.first.size() == 3);
    CATCH_REQUIRE(r4.first == std::vector<uint32_t>({0x00112233, 0x00AABBCC, 0x00DDEEFF}));
    CATCH_REQUIRE(r4.second.size() == 2);
    CATCH_REQUIRE(r4.second == std::vector<uint32_t>({0, 1}));
    Frame d4 = createData(std::vector<Color::XRGB1555>({0x0011}), {});
    Frame d5 = createData(std::vector<Color::XRGB1555>({0x00AA, 0x00DD}), {});
    std::vector<Frame> v3 = {d4, d5};
    CATCH_REQUIRE_THROWS(combineRawPixelData<uint32_t>(v3, true));
    CATCH_REQUIRE_THROWS(combineRawPixelData<uint32_t>(v3));
}

TEST_CASE("combineRawPixelDataInterleave")
{
    std::vector<Frame> v0;
    auto r0 = combineRawPixelData<uint8_t>(v0);
    CATCH_REQUIRE(r0.first.empty());
    CATCH_REQUIRE(r0.second.empty());
    Frame d0 = createData(std::vector<Color::XRGB1555>({0x0011, 0x0044}), {});
    Frame d1 = createData(std::vector<Color::XRGB1555>({0x0077, 0x00AA}), {});
    std::vector<Frame> v1 = {d0, d1};
    auto r1 = combineRawPixelData<uint8_t>(v1, true);
    CATCH_REQUIRE(r1.first.size() == 8);
    CATCH_REQUIRE(r1.first == std::vector<uint8_t>({0x11, 0x00, 0x77, 0x00, 0x44, 0x00, 0xAA, 0x00}));
    CATCH_REQUIRE(r1.second.empty());
    auto r2 = combineRawPixelData<uint16_t>(v1, true);
    CATCH_REQUIRE(r2.first.size() == 4);
    CATCH_REQUIRE(r2.first == std::vector<uint16_t>({0x0011, 0x0077, 0x0044, 0x00AA}));
    CATCH_REQUIRE(r2.second.empty());
    auto r3 = combineRawPixelData<uint32_t>(v1, true);
    CATCH_REQUIRE(r3.first.size() == 2);
    CATCH_REQUIRE(r3.first == std::vector<uint32_t>({0x00770011, 0x00AA0044}));
    CATCH_REQUIRE(r3.second.empty());
}

TEST_CASE("combineRawMapData")
{
    std::vector<Frame> v0;
    auto r0 = combineRawMapData<uint8_t>(v0);
    CATCH_REQUIRE(r0.first.empty());
    CATCH_REQUIRE(r0.second.empty());
    Frame d0 = createMapData(std::vector<uint16_t>({0x1122, 0x3344}));
    Frame d1 = createMapData(std::vector<uint16_t>({0x5566, 0x7788}));
    std::vector<Frame> v1 = {d0, d1};
    auto r1 = combineRawMapData<uint8_t>(v1);
    CATCH_STATIC_REQUIRE(std::is_same<decltype(r1.first)::value_type, uint8_t>::value);
    CATCH_STATIC_REQUIRE(std::is_same<decltype(r1.second)::value_type, uint32_t>::value);
    CATCH_REQUIRE(r1.first.size() == 8);
    CATCH_REQUIRE(r1.first == std::vector<uint8_t>({0x22, 0x11, 0x44, 0x33, 0x66, 0x55, 0x88, 0x77}));
    CATCH_REQUIRE(r1.second.size() == 2);
    CATCH_REQUIRE(r1.second == std::vector<uint32_t>({0, 4}));
    Frame d2 = createMapData(std::vector<uint16_t>({0x1122}));
    std::vector<Frame> v2 = {d2, d1};
    auto r2 = combineRawMapData<uint8_t>(v2);
    CATCH_REQUIRE(r2.first.size() == 6);
    CATCH_REQUIRE(r2.first == std::vector<uint8_t>({0x22, 0x11, 0x66, 0x55, 0x88, 0x77}));
    CATCH_REQUIRE(r2.second.size() == 2);
    CATCH_REQUIRE(r2.second == std::vector<uint32_t>({0, 2}));
    auto r3 = combineRawMapData<uint16_t, uint16_t>(v2);
    CATCH_STATIC_REQUIRE(std::is_same<decltype(r3.first)::value_type, uint16_t>::value);
    CATCH_STATIC_REQUIRE(std::is_same<decltype(r3.second)::value_type, uint16_t>::value);
    CATCH_REQUIRE(r3.first.size() == 3);
    CATCH_REQUIRE(r3.first == std::vector<uint16_t>({0x1122, 0x5566, 0x7788}));
    CATCH_REQUIRE(r3.second.size() == 2);
    CATCH_REQUIRE(r3.second == std::vector<uint16_t>({0, 1}));
    CATCH_REQUIRE_THROWS(combineRawMapData<uint32_t>(v2));
}

TEST_CASE("combineRawColorMapData")
{
    std::vector<Frame> v0;
    auto r0 = combineRawColorMapData<uint8_t>(v0);
    CATCH_REQUIRE(r0.first.empty());
    CATCH_REQUIRE(r0.second.empty());
    Frame d0 = createImageData(std::vector<uint8_t>({0x00, 0x01}), Color::Format::Paletted8, std::vector<Color::XRGB8888>({0x00112233, 0x00445566}));
    Frame d1 = createImageData(std::vector<uint8_t>({0x00, 0x01}), Color::Format::Paletted8, std::vector<Color::XRGB8888>({0x00778899, 0x00AABBCC}));
    std::vector<Frame> v1 = {d0, d1};
    auto r1 = combineRawColorMapData<uint8_t>(v1);
    CATCH_REQUIRE(r1.first.size() == 16);
    CATCH_REQUIRE(r1.first == std::vector<uint8_t>({0x33, 0x22, 0x11, 0x00, 0x66, 0x55, 0x44, 0x00, 0x99, 0x88, 0x77, 0x00, 0xCC, 0xBB, 0xAA, 0x00}));
    CATCH_REQUIRE(r1.second.size() == 2);
    CATCH_REQUIRE(r1.second == std::vector<uint32_t>({0, 8}));
    auto r2 = combineRawColorMapData<uint16_t>(v1);
    CATCH_REQUIRE(r2.first.size() == 8);
    CATCH_REQUIRE(r2.first == std::vector<uint16_t>({0x2233, 0x0011, 0x5566, 0x0044, 0x8899, 0x0077, 0xBBCC, 0x00AA}));
    CATCH_REQUIRE(r2.second.size() == 2);
    CATCH_REQUIRE(r2.second == std::vector<uint32_t>({0, 4}));
    auto r3 = combineRawColorMapData<uint32_t>(v1);
    CATCH_REQUIRE(r3.first.size() == 4);
    CATCH_REQUIRE(r3.first == std::vector<uint32_t>({0x00112233, 0x00445566, 0x00778899, 0x00AABBCC}));
    CATCH_REQUIRE(r3.second.size() == 2);
    CATCH_REQUIRE(r3.second == std::vector<uint32_t>({0, 2}));
    Frame d2 = createImageData(std::vector<uint8_t>({0x00, 0x01}), Color::Format::Paletted8, std::vector<Color::XRGB8888>({0x00112233}));
    Frame d3 = createImageData(std::vector<uint8_t>({0x00, 0x01}), Color::Format::Paletted8, std::vector<Color::XRGB8888>({0x00AABBCC, 0x00DDEEFF}));
    std::vector<Frame> v2 = {d2, d3};
    auto r4 = combineRawColorMapData<uint32_t>(v2);
    CATCH_REQUIRE(r4.first.size() == 3);
    CATCH_REQUIRE(r4.first == std::vector<uint32_t>({0x00112233, 0x00AABBCC, 0x00DDEEFF}));
    CATCH_REQUIRE(r4.second.size() == 2);
    CATCH_REQUIRE(r4.second == std::vector<uint32_t>({0, 1}));
    Frame d4 = createImageData(std::vector<uint8_t>({0x00, 0x01}), Color::Format::Paletted8, std::vector<Color::XRGB1555>({0x0011}));
    Frame d5 = createImageData(std::vector<uint8_t>({0x00, 0x01}), Color::Format::Paletted8, std::vector<Color::XRGB1555>({0x00AA, 0x00DD}));
    std::vector<Frame> v3 = {d4, d5};
    CATCH_REQUIRE_THROWS(combineRawColorMapData<uint32_t>(v3));
}

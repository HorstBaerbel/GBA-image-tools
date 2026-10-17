#include "testmacros.h"

#include "audio/audioformat.h"
#include "audio/audiohelpers.h"

using namespace Audio;
using namespace AudioHelpers;

TEST_SUITE("Audio Helpers")

TEST_CASE("toSigned16 from Signed8P")
{
    std::vector<uint8_t> rawU8 = {0, 127, 128, 255}; // 0, 127, -128, -1
    auto result = toSigned16(rawU8, SampleFormat::Signed8P);
    CATCH_REQUIRE(result.size() == 4);
    CATCH_REQUIRE(result[0] == 128);
    CATCH_REQUIRE(result[1] == 32767);
    CATCH_REQUIRE(result[2] == -32768);
    CATCH_REQUIRE(result[3] == -129);
}

TEST_CASE("toSigned16 from Unsigned8P")
{
    std::vector<uint8_t> raw = {0, 128, 255};
    auto result = toSigned16(raw, SampleFormat::Unsigned8P);
    CATCH_REQUIRE(result.size() == 3);
    CATCH_REQUIRE(result[0] == -32768);
    CATCH_REQUIRE(result[1] == 128);
    CATCH_REQUIRE(result[2] == 32767);
}

TEST_CASE("toSigned16 from Unsigned16P")
{
    std::vector<uint8_t> raw = {0x00, 0x00, 0x00, 0x80, 0xFF, 0xFF}; // 0, 32768, 65535
    auto result = toSigned16(raw, SampleFormat::Unsigned16P);
    CATCH_REQUIRE(result.size() == 3);
    CATCH_REQUIRE(result[0] == -32768);
    CATCH_REQUIRE(result[1] == 0);
    CATCH_REQUIRE(result[2] == 32767);
}

TEST_CASE("toSigned16 from Float32P")
{
    // Formula: s * 32768, clamped to [-32768, 32767]
    float f0 = 0.0F;
    float f1 = 0.5F;
    float f2 = -0.5F;
    float f3 = 1.0F;  // should clamp to 32767
    float f4 = -1.0F; // should be -32768
    std::vector<uint8_t> raw(sizeof(float) * 5);
    auto *ptr = reinterpret_cast<float *>(raw.data());
    ptr[0] = f0;
    ptr[1] = f1;
    ptr[2] = f2;
    ptr[3] = f3;
    ptr[4] = f4;
    auto result = toSigned16(raw, SampleFormat::Float32P);
    CATCH_REQUIRE(result.size() == 5);
    CATCH_REQUIRE(result[0] == 0);
    CATCH_REQUIRE(result[1] == 16384);
    CATCH_REQUIRE(result[2] == -16384);
    CATCH_REQUIRE(result[3] == 32767); // clamped
    CATCH_REQUIRE(result[4] == -32768);
}

TEST_CASE("createSampleBuffer")
{
    auto s8 = createSampleBuffer(SampleFormat::Signed8P);
    CATCH_REQUIRE(std::holds_alternative<std::vector<int8_t>>(s8));
    auto u8 = createSampleBuffer(SampleFormat::Unsigned8P);
    CATCH_REQUIRE(std::holds_alternative<std::vector<uint8_t>>(u8));
    auto s16 = createSampleBuffer(SampleFormat::Signed16P);
    CATCH_REQUIRE(std::holds_alternative<std::vector<int16_t>>(s16));
    auto u16 = createSampleBuffer(SampleFormat::Unsigned16P);
    CATCH_REQUIRE(std::holds_alternative<std::vector<uint16_t>>(u16));
    auto f32 = createSampleBuffer(SampleFormat::Float32P);
    CATCH_REQUIRE(std::holds_alternative<std::vector<float>>(f32));
}

TEST_CASE("nrOfSamples mono")
{
    SampleData data = std::vector<int16_t>{1, 2, 3, 4, 5, 6};
    CATCH_REQUIRE(nrOfSamples(data, ChannelFormat::Mono) == 6);
}

TEST_CASE("nrOfSamples stereo")
{
    // Stereo planar: L0 L1 L2 R0 R1 R2 -> 3 samples per channel
    SampleData data = std::vector<int16_t>{1, 2, 3, 4, 5, 6};
    CATCH_REQUIRE(nrOfSamples(data, ChannelFormat::Stereo) == 3);
}

TEST_CASE("nrOfSamples empty")
{
    SampleData data = std::vector<int16_t>{};
    CATCH_REQUIRE(nrOfSamples(data, ChannelFormat::Mono) == 0);
}

TEST_CASE("rawDataSize")
{
    CATCH_REQUIRE(rawDataSize(SampleData(std::vector<int8_t>{1, 2, 3})) == 3);
    CATCH_REQUIRE(rawDataSize(SampleData(std::vector<uint8_t>{1, 2, 3})) == 3);
    CATCH_REQUIRE(rawDataSize(SampleData(std::vector<int16_t>{1, 2, 3})) == 6);
    CATCH_REQUIRE(rawDataSize(SampleData(std::vector<uint16_t>{1, 2, 3})) == 6);
    CATCH_REQUIRE(rawDataSize(SampleData(std::vector<float>{1.0F, 2.0F})) == 8);
    CATCH_REQUIRE(rawDataSize(SampleData(std::vector<int16_t>{})) == 0);
}

TEST_CASE("toRawData mono int16")
{
    SampleData data = std::vector<int16_t>{0x0100, 0x0200};
    auto raw = toRawData(data, ChannelFormat::Mono);
    CATCH_REQUIRE(raw.size() == 4);
    CATCH_REQUIRE(raw[0] == 0x00);
    CATCH_REQUIRE(raw[1] == 0x01);
    CATCH_REQUIRE(raw[2] == 0x00);
    CATCH_REQUIRE(raw[3] == 0x02);
}

TEST_CASE("toRawData stereo int16")
{
    // Planar: L0=1, L1=2, R0=3, R1=4
    SampleData data = std::vector<int16_t>{1, 2, 3, 4};
    auto raw = toRawData(data, ChannelFormat::Stereo);
    CATCH_REQUIRE(raw.size() == 8);
    auto *ptr = reinterpret_cast<const int16_t *>(raw.data());
    CATCH_REQUIRE(ptr[0] == 1); // L0
    CATCH_REQUIRE(ptr[1] == 2); // L1
    CATCH_REQUIRE(ptr[2] == 3); // R0
    CATCH_REQUIRE(ptr[3] == 4); // R1
}

TEST_CASE("toRawInterleavedData mono")
{
    SampleData data = std::vector<int16_t>{1, 2, 3};
    auto raw = toRawInterleavedData(data, ChannelFormat::Mono);
    CATCH_REQUIRE(raw.size() == 6);
    // Should be same as toRawData for mono
    auto rawDirect = toRawData(data, ChannelFormat::Mono);
    CATCH_REQUIRE(raw == rawDirect);
}

TEST_CASE("toRawInterleavedData stereo")
{
    // Planar: L0=1, L1=2, R0=3, R1=4
    // Interleaved: L0 R0 L1 R1 = 1, 3, 2, 4
    SampleData data = std::vector<int16_t>{1, 2, 3, 4};
    auto raw = toRawInterleavedData(data, ChannelFormat::Stereo);
    CATCH_REQUIRE(raw.size() == 8);
    auto *ptr = reinterpret_cast<const int16_t *>(raw.data());
    CATCH_REQUIRE(ptr[0] == 1); // L0
    CATCH_REQUIRE(ptr[1] == 3); // R0
    CATCH_REQUIRE(ptr[2] == 2); // L1
    CATCH_REQUIRE(ptr[3] == 4); // R1
}

TEST_CASE("toRawInterleavedData stereo uint8")
{
    // Planar: L0=10, L1=20, R0=30, R1=40
    // Interleaved: 10, 30, 20, 40
    SampleData data = std::vector<uint8_t>{10, 20, 30, 40};
    auto raw = toRawInterleavedData(data, ChannelFormat::Stereo);
    CATCH_REQUIRE(raw.size() == 4);
    CATCH_REQUIRE(raw[0] == 10);
    CATCH_REQUIRE(raw[1] == 30);
    CATCH_REQUIRE(raw[2] == 20);
    CATCH_REQUIRE(raw[3] == 40);
}

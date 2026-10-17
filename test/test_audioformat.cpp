#include "testmacros.h"

#include "audio/audioformat.h"

using namespace Audio;

TEST_SUITE("Audio Format")

TEST_CASE("formatInfo ChannelFormat")
{
    // Mono
    auto monoInfo = formatInfo(ChannelFormat::Mono);
    CATCH_REQUIRE(monoInfo.format == ChannelFormat::Mono);
    CATCH_REQUIRE(monoInfo.id == "mono");
    CATCH_REQUIRE(monoInfo.nrOfChannels == 1);
    // Stereo
    auto stereoInfo = formatInfo(ChannelFormat::Stereo);
    CATCH_REQUIRE(stereoInfo.format == ChannelFormat::Stereo);
    CATCH_REQUIRE(stereoInfo.id == "stereo");
    CATCH_REQUIRE(stereoInfo.nrOfChannels == 2);
    // Unknown
    auto unknownInfo = formatInfo(ChannelFormat::Unknown);
    CATCH_REQUIRE(unknownInfo.format == ChannelFormat::Unknown);
    CATCH_REQUIRE(unknownInfo.id.empty());
    CATCH_REQUIRE(unknownInfo.nrOfChannels == 0);
}

TEST_CASE("findChannelFormat")
{
    CATCH_REQUIRE(findChannelFormat("mono") == ChannelFormat::Mono);
    CATCH_REQUIRE(findChannelFormat("stereo") == ChannelFormat::Stereo);
    CATCH_REQUIRE(findChannelFormat("unknown") == ChannelFormat::Unknown);
    CATCH_REQUIRE(findChannelFormat("") == ChannelFormat::Unknown);
}

TEST_CASE("formatInfo SampleFormat")
{
    // Signed 8-bit planar
    auto s8pInfo = formatInfo(SampleFormat::Signed8P);
    CATCH_REQUIRE(s8pInfo.format == SampleFormat::Signed8P);
    CATCH_REQUIRE(s8pInfo.id == "s8p");
    CATCH_REQUIRE(s8pInfo.bitsPerSample == 8);
    CATCH_REQUIRE(s8pInfo.isSigned == true);
    // Unsigned 8-bit planar
    auto u8pInfo = formatInfo(SampleFormat::Unsigned8P);
    CATCH_REQUIRE(u8pInfo.format == SampleFormat::Unsigned8P);
    CATCH_REQUIRE(u8pInfo.id == "u8p");
    CATCH_REQUIRE(u8pInfo.bitsPerSample == 8);
    CATCH_REQUIRE(u8pInfo.isSigned == false);
    // Signed 16-bit planar
    auto s16pInfo = formatInfo(SampleFormat::Signed16P);
    CATCH_REQUIRE(s16pInfo.format == SampleFormat::Signed16P);
    CATCH_REQUIRE(s16pInfo.id == "s16p");
    CATCH_REQUIRE(s16pInfo.bitsPerSample == 16);
    CATCH_REQUIRE(s16pInfo.isSigned == true);
    // Unsigned 16-bit planar
    auto u16pInfo = formatInfo(SampleFormat::Unsigned16P);
    CATCH_REQUIRE(u16pInfo.format == SampleFormat::Unsigned16P);
    CATCH_REQUIRE(u16pInfo.id == "u16p");
    CATCH_REQUIRE(u16pInfo.bitsPerSample == 16);
    CATCH_REQUIRE(u16pInfo.isSigned == false);
    // Float 32-bit planar
    auto f32pInfo = formatInfo(SampleFormat::Float32P);
    CATCH_REQUIRE(f32pInfo.format == SampleFormat::Float32P);
    CATCH_REQUIRE(f32pInfo.id == "f32p");
    CATCH_REQUIRE(f32pInfo.bitsPerSample == 32);
    CATCH_REQUIRE(f32pInfo.isSigned == true);
}

TEST_CASE("findSampleFormat by id")
{
    CATCH_REQUIRE(findSampleFormat("s8p") == SampleFormat::Signed8P);
    CATCH_REQUIRE(findSampleFormat("u8p") == SampleFormat::Unsigned8P);
    CATCH_REQUIRE(findSampleFormat("s16p") == SampleFormat::Signed16P);
    CATCH_REQUIRE(findSampleFormat("u16p") == SampleFormat::Unsigned16P);
    CATCH_REQUIRE(findSampleFormat("f32p") == SampleFormat::Float32P);
    CATCH_REQUIRE(findSampleFormat("bad") == SampleFormat::Unknown);
}

TEST_CASE("findSampleFormat by bits and signedness")
{
    CATCH_REQUIRE(findSampleFormat(8, true) == SampleFormat::Signed8P);
    CATCH_REQUIRE(findSampleFormat(8, false) == SampleFormat::Unsigned8P);
    CATCH_REQUIRE(findSampleFormat(16, true) == SampleFormat::Signed16P);
    CATCH_REQUIRE(findSampleFormat(16, false) == SampleFormat::Unsigned16P);
    CATCH_REQUIRE(findSampleFormat(32, true) == SampleFormat::Float32P);
    CATCH_REQUIRE(findSampleFormat(0, true) == SampleFormat::Unknown);
    CATCH_REQUIRE(findSampleFormat(64, false) == SampleFormat::Unknown);
}

TEST_CASE("checkSampleFormat")
{
    // Signed 8-bit
    SampleData s8 = std::vector<int8_t>{1, 2, 3};
    CATCH_REQUIRE(checkSampleFormat(s8, SampleFormat::Signed8P));
    CATCH_REQUIRE_FALSE(checkSampleFormat(s8, SampleFormat::Unsigned8P));
    // Unsigned 8-bit
    SampleData u8 = std::vector<uint8_t>{1, 2, 3};
    CATCH_REQUIRE(checkSampleFormat(u8, SampleFormat::Unsigned8P));
    CATCH_REQUIRE_FALSE(checkSampleFormat(u8, SampleFormat::Signed8P));
    // Signed 16-bit
    SampleData s16 = std::vector<int16_t>{100, -200};
    CATCH_REQUIRE(checkSampleFormat(s16, SampleFormat::Signed16P));
    CATCH_REQUIRE_FALSE(checkSampleFormat(s16, SampleFormat::Unsigned16P));
    // Unsigned 16-bit
    SampleData u16 = std::vector<uint16_t>{100, 200};
    CATCH_REQUIRE(checkSampleFormat(u16, SampleFormat::Unsigned16P));
    CATCH_REQUIRE_FALSE(checkSampleFormat(u16, SampleFormat::Signed16P));
    // Float 32-bit
    SampleData f32 = std::vector<float>{0.5F, -0.5F};
    CATCH_REQUIRE(checkSampleFormat(f32, SampleFormat::Float32P));
    CATCH_REQUIRE_FALSE(checkSampleFormat(f32, SampleFormat::Signed16P));
}

TEST_CASE("isEmpty SampleData")
{
    // Empty vectors
    CATCH_REQUIRE(isEmpty(SampleData(std::vector<int8_t>{})));
    CATCH_REQUIRE(isEmpty(SampleData(std::vector<uint8_t>{})));
    CATCH_REQUIRE(isEmpty(SampleData(std::vector<int16_t>{})));
    CATCH_REQUIRE(isEmpty(SampleData(std::vector<uint16_t>{})));
    CATCH_REQUIRE(isEmpty(SampleData(std::vector<float>{})));
    // Non-empty vectors
    CATCH_REQUIRE_FALSE(isEmpty(SampleData(std::vector<int8_t>{1})));
    CATCH_REQUIRE_FALSE(isEmpty(SampleData(std::vector<uint8_t>{1})));
    CATCH_REQUIRE_FALSE(isEmpty(SampleData(std::vector<int16_t>{1})));
    CATCH_REQUIRE_FALSE(isEmpty(SampleData(std::vector<uint16_t>{1})));
    CATCH_REQUIRE_FALSE(isEmpty(SampleData(std::vector<float>{1.0F})));
}

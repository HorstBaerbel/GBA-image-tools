#include "testmacros.h"

#include "audio/audiostructs.h"

using namespace Audio;

TEST_SUITE("Audio Structs")

TEST_CASE("isEmptyFrame")
{
    // Default frame: empty data, unknown formats, zero sample rate -> empty
    Frame frame;
    CATCH_REQUIRE(isEmpty(frame));
    // Data present but channel format unknown -> still empty
    frame.data = std::vector<int16_t>{1, 2, 3};
    frame.info.channelFormat = ChannelFormat::Mono;
    frame.info.sampleFormat = SampleFormat::Signed16P;
    frame.info.sampleRateHz = 44100;
    CATCH_REQUIRE_FALSE(isEmpty(frame));
    // Now make channel format unknown -> empty
    frame.info.channelFormat = ChannelFormat::Unknown;
    CATCH_REQUIRE(isEmpty(frame));
    // Restore channel, make sample format unknown -> empty
    frame.info.channelFormat = ChannelFormat::Mono;
    frame.info.sampleFormat = SampleFormat::Unknown;
    CATCH_REQUIRE(isEmpty(frame));
    // Restore sample format, make sample rate zero -> empty
    frame.info.sampleFormat = SampleFormat::Signed16P;
    frame.info.sampleRateHz = 0;
    CATCH_REQUIRE(isEmpty(frame));
    // All valid, but empty data -> empty
    frame.info.sampleRateHz = 44100;
    frame.data = std::vector<int16_t>{};
    CATCH_REQUIRE(isEmpty(frame));
}

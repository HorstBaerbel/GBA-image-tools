#include "testmacros.h"

#include "audio/audioformat.h"
#include "audio/audiostructs.h"
#include "audio/samplebuffer.h"

using namespace Audio;

TEST_SUITE("Sample Buffer")

TEST_CASE("Constructor validation")
{
    // Bad channel format
    CATCH_REQUIRE_THROWS_AS(SampleBuffer(ChannelFormat::Unknown, 44100, SampleFormat::Signed16P), std::runtime_error);
    // Bad sample rate
    CATCH_REQUIRE_THROWS_AS(SampleBuffer(ChannelFormat::Mono, 0, SampleFormat::Signed16P), std::runtime_error);
    // Bad sample format
    CATCH_REQUIRE_THROWS_AS(SampleBuffer(ChannelFormat::Mono, 44100, SampleFormat::Unknown), std::runtime_error);
    // Valid construction
    SampleBuffer buf(ChannelFormat::Mono, 44100, SampleFormat::Signed16P);
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 0);
}

TEST_CASE("push_back and nrOfSamplesPerChannel mono")
{
    SampleBuffer buf(ChannelFormat::Mono, 44100, SampleFormat::Signed16P);
    Frame frame;
    frame.info.channelFormat = ChannelFormat::Mono;
    frame.info.sampleRateHz = 44100;
    frame.info.sampleFormat = SampleFormat::Signed16P;
    frame.data = std::vector<int16_t>{1, 2, 3, 4};
    buf.push_back(frame);
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 4);
    // Push more
    Frame frame2;
    frame2.info.channelFormat = ChannelFormat::Mono;
    frame2.info.sampleRateHz = 44100;
    frame2.info.sampleFormat = SampleFormat::Signed16P;
    frame2.data = std::vector<int16_t>{5, 6};
    buf.push_back(frame2);
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 6);
}

TEST_CASE("push_back and nrOfSamplesPerChannel stereo")
{
    SampleBuffer buf(ChannelFormat::Stereo, 44100, SampleFormat::Signed16P);
    // Planar stereo: L0 L1 R0 R1 = 2 samples per channel
    Frame frame;
    frame.info.channelFormat = ChannelFormat::Stereo;
    frame.info.sampleRateHz = 44100;
    frame.info.sampleFormat = SampleFormat::Signed16P;
    frame.data = std::vector<int16_t>{1, 2, 3, 4};
    buf.push_back(frame);
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 2);
}

TEST_CASE("push_back format mismatch throws")
{
    SampleBuffer buf(ChannelFormat::Mono, 44100, SampleFormat::Signed16P);
    // Wrong channel format
    Frame frame;
    frame.info.channelFormat = ChannelFormat::Stereo;
    frame.info.sampleRateHz = 44100;
    frame.info.sampleFormat = SampleFormat::Signed16P;
    frame.data = std::vector<int16_t>{1, 2, 3, 4};
    CATCH_REQUIRE_THROWS_AS(buf.push_back(frame), std::runtime_error);
    // Wrong sample rate
    frame.info.channelFormat = ChannelFormat::Mono;
    frame.info.sampleRateHz = 8000;
    CATCH_REQUIRE_THROWS_AS(buf.push_back(frame), std::runtime_error);
    // Wrong sample format
    frame.info.sampleRateHz = 44100;
    frame.info.sampleFormat = SampleFormat::Unsigned16P;
    frame.data = std::vector<uint16_t>{1, 2};
    CATCH_REQUIRE_THROWS_AS(buf.push_back(frame), std::runtime_error);
}

TEST_CASE("push_silence mono")
{
    SampleBuffer buf(ChannelFormat::Mono, 44100, SampleFormat::Signed16P);
    Frame frame;
    frame.info.channelFormat = ChannelFormat::Mono;
    frame.info.sampleRateHz = 44100;
    frame.info.sampleFormat = SampleFormat::Signed16P;
    frame.data = std::vector<int16_t>{100, 200};
    buf.push_back(frame);
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 2);
    // Push 3 silence samples (zeros for signed)
    buf.push_silence(3);
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 5);
    auto result = buf.pop_front(5);
    auto &samples = std::get<std::vector<int16_t>>(result.data);
    CATCH_REQUIRE(samples.size() == 5);
    CATCH_REQUIRE(samples[0] == 100);
    CATCH_REQUIRE(samples[1] == 200);
    CATCH_REQUIRE(samples[2] == 0);
    CATCH_REQUIRE(samples[3] == 0);
    CATCH_REQUIRE(samples[4] == 0);
}

TEST_CASE("push_silence unsigned8")
{
    SampleBuffer buf(ChannelFormat::Mono, 44100, SampleFormat::Unsigned8P);
    Frame frame;
    frame.info.channelFormat = ChannelFormat::Mono;
    frame.info.sampleRateHz = 44100;
    frame.info.sampleFormat = SampleFormat::Unsigned8P;
    frame.data = std::vector<uint8_t>{50, 200};
    buf.push_back(frame);
    buf.push_silence(2);
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 4);
    auto result = buf.pop_front(4);
    auto &samples = std::get<std::vector<uint8_t>>(result.data);
    CATCH_REQUIRE(samples.size() == 4);
    CATCH_REQUIRE(samples[0] == 50);
    CATCH_REQUIRE(samples[1] == 200);
    // Silence for unsigned 8-bit is 128
    CATCH_REQUIRE(samples[2] == 128);
    CATCH_REQUIRE(samples[3] == 128);
}

TEST_CASE("pop_front mono")
{
    SampleBuffer buf(ChannelFormat::Mono, 44100, SampleFormat::Signed16P);
    Frame frame;
    frame.info.channelFormat = ChannelFormat::Mono;
    frame.info.sampleRateHz = 44100;
    frame.info.sampleFormat = SampleFormat::Signed16P;
    frame.data = std::vector<int16_t>{1, 2, 3, 4, 5, 6};
    buf.push_back(frame);
    // Pop first 3 samples
    auto result = buf.pop_front(3);
    auto &samples = std::get<std::vector<int16_t>>(result.data);
    CATCH_REQUIRE(samples.size() == 3);
    CATCH_REQUIRE(samples[0] == 1);
    CATCH_REQUIRE(samples[1] == 2);
    CATCH_REQUIRE(samples[2] == 3);
    // Remaining should be 3 samples
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 3);
    // Pop remaining
    auto result2 = buf.pop_front(3);
    auto &samples2 = std::get<std::vector<int16_t>>(result2.data);
    CATCH_REQUIRE(samples2.size() == 3);
    CATCH_REQUIRE(samples2[0] == 4);
    CATCH_REQUIRE(samples2[1] == 5);
    CATCH_REQUIRE(samples2[2] == 6);
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 0);
}

TEST_CASE("pop_front stereo")
{
    SampleBuffer buf(ChannelFormat::Stereo, 44100, SampleFormat::Signed16P);
    // Planar: L0=1, L1=2, L2=3, R0=10, R1=20, R2=30
    Frame frame;
    frame.info.channelFormat = ChannelFormat::Stereo;
    frame.info.sampleRateHz = 44100;
    frame.info.sampleFormat = SampleFormat::Signed16P;
    frame.data = std::vector<int16_t>{1, 2, 3, 10, 20, 30};
    buf.push_back(frame);
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 3);
    // Pop 2 samples per channel
    auto result = buf.pop_front(2);
    auto &samples = std::get<std::vector<int16_t>>(result.data);
    CATCH_REQUIRE(samples.size() == 4); // 2 per channel * 2 channels
    // Planar output: L0, L1, R0, R1
    CATCH_REQUIRE(samples[0] == 1);
    CATCH_REQUIRE(samples[1] == 2);
    CATCH_REQUIRE(samples[2] == 10);
    CATCH_REQUIRE(samples[3] == 20);
    // Remaining: L2=3, R2=30 -> 1 sample per channel
    CATCH_REQUIRE(buf.nrOfSamplesPerChannel() == 1);
}

TEST_CASE("pop_front not enough samples throws")
{
    SampleBuffer buf(ChannelFormat::Mono, 44100, SampleFormat::Signed16P);
    Frame frame;
    frame.info.channelFormat = ChannelFormat::Mono;
    frame.info.sampleRateHz = 44100;
    frame.info.sampleFormat = SampleFormat::Signed16P;
    frame.data = std::vector<int16_t>{1, 2};
    buf.push_back(frame);
    // Try to pop more than available
    CATCH_REQUIRE_THROWS_AS(buf.pop_front(5), std::runtime_error);
}

TEST_CASE("pop_front zero samples throws")
{
    SampleBuffer buf(ChannelFormat::Mono, 44100, SampleFormat::Signed16P);
    Frame frame;
    frame.info.channelFormat = ChannelFormat::Mono;
    frame.info.sampleRateHz = 44100;
    frame.info.sampleFormat = SampleFormat::Signed16P;
    frame.data = std::vector<int16_t>{1, 2};
    buf.push_back(frame);
    CATCH_REQUIRE_THROWS_AS(buf.pop_front(0), std::runtime_error);
}

TEST_CASE("Frame info in pop_front result")
{
    SampleBuffer buf(ChannelFormat::Stereo, 8000, SampleFormat::Float32P);
    Frame frame;
    frame.info.channelFormat = ChannelFormat::Stereo;
    frame.info.sampleRateHz = 8000;
    frame.info.sampleFormat = SampleFormat::Float32P;
    frame.data = std::vector<float>{1.0F, 2.0F, 3.0F, 4.0F};
    buf.push_back(frame);
    auto result = buf.pop_front(2);
    CATCH_REQUIRE(result.info.channelFormat == ChannelFormat::Stereo);
    CATCH_REQUIRE(result.info.sampleRateHz == 8000);
    CATCH_REQUIRE(result.info.sampleFormat == SampleFormat::Float32P);
    CATCH_REQUIRE(result.nrOfSamples == 2);
}

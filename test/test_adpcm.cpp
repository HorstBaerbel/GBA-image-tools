#include "testmacros.h"

#include "audio/audioformat.h"
#include "audio/audiostructs.h"
#include "audio_codec/adpcm.h"
#include "audio_codec/adpcm_structs.h"
#include "statistics/statistics.h"

#include <cmath>

using namespace Audio;

TEST_SUITE("ADPCM")

// Helper to create a SampleData from int16_t samples
static SampleData makeSampleData(std::vector<int16_t> samples)
{
    return std::move(samples);
}

TEST_CASE("Adpcm constructor mono")
{
    Adpcm adpcm(ChannelFormat::Mono, 44100);
    // Just verify it constructs without throwing
    CATCH_REQUIRE(true);
}

TEST_CASE("Adpcm constructor stereo")
{
    Adpcm adpcm(ChannelFormat::Stereo, 44100);
    CATCH_REQUIRE(true);
}

TEST_CASE("encode mono s16p to adpcm")
{
    Adpcm adpcm(ChannelFormat::Mono, 44100);
    // Create a simple sine-like signal
    std::vector<int16_t> samples;
    for (int i = 0; i < 100; ++i)
    {
        samples.push_back(static_cast<int16_t>(1000 * sin(2.0 * M_PI * i / 50.0)));
    }
    auto encoded = adpcm.encode(makeSampleData(std::move(samples)));
    // Should produce non-empty ADPCM data (header + compressed samples)
    CATCH_REQUIRE(!encoded.empty());
    // The first 4 bytes should be the frame header
    CATCH_REQUIRE(encoded.size() >= sizeof(AdpcmFrameHeader));
    // Verify the frame header contains correct info
    auto header = AdpcmFrameHeader::read(encoded.data());
    CATCH_REQUIRE(header.nrOfChannels == 1);
    CATCH_REQUIRE(header.pcmBitsPerSample == 16);
}

TEST_CASE("encode stereo s16p to adpcm")
{
    Adpcm adpcm(ChannelFormat::Stereo, 44100);
    // Stereo: L and R channels in planar format (all L samples first, then all R)
    std::vector<int16_t> samples;
    for (int i = 0; i < 50; ++i)
    {
        samples.push_back(static_cast<int16_t>(500 * sin(2.0 * M_PI * i / 25.0))); // L
    }
    for (int i = 0; i < 50; ++i)
    {
        samples.push_back(static_cast<int16_t>(500 * cos(2.0 * M_PI * i / 25.0))); // R
    }
    auto encoded = adpcm.encode(makeSampleData(std::move(samples)));
    CATCH_REQUIRE(!encoded.empty());
    CATCH_REQUIRE(encoded.size() >= sizeof(AdpcmFrameHeader));
    auto header = AdpcmFrameHeader::read(encoded.data());
    CATCH_REQUIRE(header.nrOfChannels == 2);
}

TEST_CASE("encode silence produces valid output")
{
    Adpcm adpcm(ChannelFormat::Mono, 44100);
    std::vector<int16_t> samples(100, 0);
    auto encoded = adpcm.encode(makeSampleData(std::move(samples)));
    CATCH_REQUIRE(!encoded.empty());
}

TEST_CASE("encode/decode roundtrip mono")
{
    Adpcm adpcm(ChannelFormat::Mono, 44100);
    // Create a simple signal
    std::vector<int16_t> samples;
    for (int i = 0; i < 100; ++i)
    {
        samples.push_back(static_cast<int16_t>(2000 * sin(2.0 * M_PI * i / 50.0)));
    }
    auto encoded = adpcm.encode(makeSampleData(samples));
    CATCH_REQUIRE(!encoded.empty());
    // Decode back (static method)
    auto decoded = Adpcm::decode(encoded);
    // Should be int16_t format (variant index 2)
    CATCH_REQUIRE(decoded.index() == 2);
    auto &decodedSamples = std::get<std::vector<int16_t>>(decoded);
    // ADPCM is lossy, so we check that the signal is roughly similar
    CATCH_REQUIRE(decodedSamples.size() >= 100);
    // Check correlation - the decoded signal should be correlated with the original
    double dot = 0.0;
    double magOrig = 0.0;
    double magDec = 0.0;
    int n = std::min(100, static_cast<int>(decodedSamples.size()));
    for (int i = 0; i < n; ++i)
    {
        dot += samples[i] * decodedSamples[i];
        magOrig += samples[i] * samples[i];
        magDec += decodedSamples[i] * decodedSamples[i];
    }
    double correlation = dot / std::sqrt(magOrig * magDec);
    CATCH_REQUIRE(correlation > 0.7); // Should be reasonably correlated
}

TEST_CASE("encode/decode roundtrip stereo")
{
    Adpcm adpcm(ChannelFormat::Stereo, 44100);
    std::vector<int16_t> samples;
    for (int i = 0; i < 50; ++i)
    {
        samples.push_back(static_cast<int16_t>(1000 * sin(2.0 * M_PI * i / 25.0)));
    }
    for (int i = 0; i < 50; ++i)
    {
        samples.push_back(static_cast<int16_t>(1000 * cos(2.0 * M_PI * i / 25.0)));
    }
    auto encoded = adpcm.encode(makeSampleData(std::move(samples)));
    CATCH_REQUIRE(!encoded.empty());
    auto decoded = Adpcm::decode(encoded);
    CATCH_REQUIRE(decoded.index() == 2);
    auto &decodedSamples = std::get<std::vector<int16_t>>(decoded);
    CATCH_REQUIRE(decodedSamples.size() >= 100); // 50 per channel * 2 channels
}

TEST_CASE("encode with non-int16 input throws")
{
    Adpcm adpcm(ChannelFormat::Mono, 44100);
    // Float32P input should throw since encode requires int16_t
    std::vector<float> floatSamples(50, 0.5F);
    SampleData sampleData = std::move(floatSamples);
    CATCH_REQUIRE_THROWS_AS(adpcm.encode(sampleData), std::runtime_error);
}

TEST_CASE("encode with unsigned16p input throws")
{
    Adpcm adpcm(ChannelFormat::Mono, 44100);
    std::vector<uint16_t> uintSamples(50, 32768);
    SampleData sampleData = std::move(uintSamples);
    CATCH_REQUIRE_THROWS_AS(adpcm.encode(sampleData), std::runtime_error);
}

TEST_CASE("encode with unsigned8p input throws")
{
    Adpcm adpcm(ChannelFormat::Mono, 44100);
    std::vector<uint8_t> uint8Samples(50, 128);
    SampleData sampleData = std::move(uint8Samples);
    CATCH_REQUIRE_THROWS_AS(adpcm.encode(sampleData), std::runtime_error);
}

TEST_CASE("encode with signed8p input throws")
{
    Adpcm adpcm(ChannelFormat::Mono, 44100);
    std::vector<int8_t> int8Samples(50, 64);
    SampleData sampleData = std::move(int8Samples);
    CATCH_REQUIRE_THROWS_AS(adpcm.encode(sampleData), std::runtime_error);
}

TEST_CASE("AdpcmFrameHeader write/read roundtrip")
{
    AdpcmFrameHeader header;
    header.flags = 0;
    header.nrOfChannels = 2;
    header.pcmBitsPerSample = 16;
    header.adpcmBitsPerSample = 4;
    header.uncompressedSize = 4096;
    uint8_t buffer[sizeof(AdpcmFrameHeader)] = {};
    AdpcmFrameHeader::write(buffer, header);
    auto readBack = AdpcmFrameHeader::read(buffer);
    CATCH_REQUIRE(readBack.nrOfChannels == 2);
    CATCH_REQUIRE(readBack.pcmBitsPerSample == 16);
    CATCH_REQUIRE(readBack.adpcmBitsPerSample == 4);
    CATCH_REQUIRE(readBack.uncompressedSize == 4096);
}

TEST_CASE("AdpcmFrameHeader mono")
{
    AdpcmFrameHeader header;
    header.flags = 0;
    header.nrOfChannels = 1;
    header.pcmBitsPerSample = 16;
    header.adpcmBitsPerSample = 4;
    header.uncompressedSize = 2048;
    uint8_t buffer[sizeof(AdpcmFrameHeader)] = {};
    AdpcmFrameHeader::write(buffer, header);
    auto readBack = AdpcmFrameHeader::read(buffer);
    CATCH_REQUIRE(readBack.nrOfChannels == 1);
    CATCH_REQUIRE(readBack.pcmBitsPerSample == 16);
    CATCH_REQUIRE(readBack.uncompressedSize == 2048);
}

#include "audiohelpers.h"

#include "exception.h"

#include <algorithm>
#include <cstring>

namespace AudioHelpers
{

    auto toSigned16(const std::vector<uint8_t> &samples, Audio::SampleFormat sampleFormat) -> std::vector<int16_t>
    {
        std::vector<int16_t> result;
        REQUIRE(samples.empty(), std::runtime_error, "Sample can not be empty");
        switch (sampleFormat)
        {
        case Audio::SampleFormat::Signed8:
            std::transform(samples.cbegin(), samples.cend(), std::back_inserter(result), [](auto s)
                           { return static_cast<int16_t>((static_cast<int32_t>(s) + 128) * 257 - 32768); });
            break;
        case Audio::SampleFormat::Unsigned8:
            std::transform(samples.cbegin(), samples.cend(), std::back_inserter(result), [](auto s)
                           { return static_cast<int16_t>(static_cast<int32_t>(s) * 257 - 32768); });
            break;
        case Audio::SampleFormat::Signed16:
            REQUIRE(samples.size() % 2 == 0, std::runtime_error, "Size of raw int16_t sample data must a multiple of 2");
            result.resize(samples.size() / 2);
            std::memcpy(result.data(), samples.data(), samples.size());
            break;
        case Audio::SampleFormat::Unsigned16:
        {
            REQUIRE(samples.size() % 2 == 0, std::runtime_error, "Size of raw uint16_t sample data must a multiple of 2");
            auto samplesPtr16 = reinterpret_cast<const uint16_t *>(samples.data());
            for (std::size_t i = 0; i < samples.size() / 2; ++i)
            {
                result.push_back(static_cast<int16_t>(static_cast<int32_t>(samplesPtr16[i]) - 32768));
            }
            break;
        }
        case Audio::SampleFormat::Float32:
        {
            REQUIRE(samples.size() % 4 == 0, std::runtime_error, "Size of raw float sample data must be a multiple of 4");
            auto samplesPtr32 = reinterpret_cast<const float *>(samples.data());
            for (std::size_t i = 0; i < samples.size() / 4; ++i)
            {
                // float audio samples are in the range [-1,1]
                // this can only be converted by either:
                // * giving up them mapping of 0.0 -> 0 or
                // * having to clamp the upper bound to 32767
                // we choose the later here
                float s = static_cast<float>(samplesPtr32[i]) * 32768;
                s = s > 32767 ? 32767 : s;
                result.push_back(static_cast<int16_t>(s));
            }
            break;
        }

        default:
            THROW(std::runtime_error, "Bad sample format");
        }
        return result;
    }

}

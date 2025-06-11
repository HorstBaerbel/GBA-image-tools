#include "audioformat.h"

#include "exception.h"

#include <algorithm>
#include <map>

namespace Audio
{

    static const std::map<ChannelFormat, ChannelFormatInfo> ChannelFormatInfoMap = {
        {ChannelFormat::Unknown, {ChannelFormat::Unknown, "Unknown", 0}},
        {ChannelFormat::Mono, {ChannelFormat::Mono, "Mono", 1}},
        {ChannelFormat::Stereo, {ChannelFormat::Stereo, "Stereo", 2}}};

    static const std::map<SampleFormat, SampleFormatInfo> SampleFormatInfoMap = {
        {SampleFormat::Unknown, {SampleFormat::Unknown, "Unknown", 0, false}},
        {SampleFormat::Signed8, {SampleFormat::Signed8, "Signed 8-bit", 8, true}},
        {SampleFormat::Unsigned8, {SampleFormat::Unsigned8, "Unsigned 8-bit", 8, false}},
        {SampleFormat::Signed16, {SampleFormat::Signed16, "Signed 16-bit", 16, true}},
        {SampleFormat::Unsigned16, {SampleFormat::Unsigned16, "Unsigned 16-bit", 16, false}},
        {SampleFormat::Float32, {SampleFormat::Float32, "Float 32-bit", 32, true}}};

    auto formatInfo(ChannelFormat channelformat) -> const ChannelFormatInfo &
    {
        return ChannelFormatInfoMap.at(channelformat);
    }

    auto formatInfo(SampleFormat sampleformat) -> const SampleFormatInfo &
    {
        return SampleFormatInfoMap.at(sampleformat);
    }

    auto findFormat(uint32_t bitsPerSample, bool isSigned) -> SampleFormat
    {
        auto sfimIt = std::find_if(SampleFormatInfoMap.cbegin(), SampleFormatInfoMap.cend(), [bitsPerSample, isSigned](const auto &f)
                                   { return f.second.bitsPerSample == bitsPerSample && f.second.isSigned == isSigned; });
        if (sfimIt != SampleFormatInfoMap.cend())
        {
            return sfimIt->first;
        }
        return SampleFormat::Unknown;
    }

}

#include "audioformat.h"

#include "exception.h"

#include <algorithm>
#include <map>

namespace Audio
{

    auto rawSampleDataSize(const SampleData &samples) -> uint32_t
    {
        return std::visit([](auto data)
                          { 
            using T = std::decay_t<decltype(data)>::value_type;
            return sizeof(T) * data.size(); }, samples);
    }

    static const std::map<ChannelFormat, ChannelFormatInfo> ChannelFormatInfoMap = {
        {ChannelFormat::Unknown, {ChannelFormat::Unknown, "Unknown", "", 0}},
        {ChannelFormat::Mono, {ChannelFormat::Mono, "Mono", "mono", 1}},
        {ChannelFormat::Stereo, {ChannelFormat::Stereo, "Stereo", "stereo", 2}}};

    static const std::map<SampleFormat, SampleFormatInfo> SampleFormatInfoMap = {
        {SampleFormat::Unknown, {SampleFormat::Unknown, "Unknown", "", 0, false}},
        {SampleFormat::Signed8P, {SampleFormat::Signed8P, "Signed 8-bit planar", "s8p", 8, true}},
        {SampleFormat::Unsigned8P, {SampleFormat::Unsigned8P, "Unsigned 8-bit planar", "u8p", 8, false}},
        {SampleFormat::Signed16P, {SampleFormat::Signed16P, "Signed 16-bit planar", "s16p", 16, true}},
        {SampleFormat::Unsigned16P, {SampleFormat::Unsigned16P, "Unsigned 16-bit planar", "u16p", 16, false}},
        {SampleFormat::Float32P, {SampleFormat::Float32P, "Float 32-bit planar", "f32p", 32, true}}};

    auto formatInfo(ChannelFormat channelformat) -> const ChannelFormatInfo &
    {
        return ChannelFormatInfoMap.at(channelformat);
    }

    auto findChannelFormat(const std::string &id) -> ChannelFormat
    {
        auto cfimIt = std::find_if(ChannelFormatInfoMap.cbegin(), ChannelFormatInfoMap.cend(), [id](const auto &f)
                                   { return f.second.id == id; });
        if (cfimIt != ChannelFormatInfoMap.cend())
        {
            return cfimIt->first;
        }
        return ChannelFormat::Unknown;
    }

    auto formatInfo(SampleFormat sampleformat) -> const SampleFormatInfo &
    {
        return SampleFormatInfoMap.at(sampleformat);
    }

    auto findSampleFormat(const std::string &id) -> SampleFormat
    {
        auto sfimIt = std::find_if(SampleFormatInfoMap.cbegin(), SampleFormatInfoMap.cend(), [id](const auto &f)
                                   { return f.second.id == id; });
        if (sfimIt != SampleFormatInfoMap.cend())
        {
            return sfimIt->first;
        }
        return SampleFormat::Unknown;
    }

    auto findSampleFormat(uint32_t bitsPerSample, bool isSigned) -> SampleFormat
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

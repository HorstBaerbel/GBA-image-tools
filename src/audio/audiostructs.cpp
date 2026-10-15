#pragma once

#include "audiostructs.h"

namespace Audio
{
    auto isEmpty(const Frame &frame) -> bool
    {
        return isEmpty(frame.data) ||
               frame.info.channelFormat == ChannelFormat::Unknown ||
               frame.info.sampleFormat == SampleFormat::Unknown ||
               frame.info.sampleRateHz == 0;
    }
}
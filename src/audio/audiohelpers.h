#pragma once

#include "sampleformat.h"

#include <cstdint>
#include <vector>

namespace AudioHelpers
{

    /// @brief Convert audio data to int16_t
    /// @param samples Audio samples
    /// @param sampleFormat Sample format
    /// @return int16_t sample data
    auto toSigned16(const std::vector<uint8_t> &samples, Audio::SampleFormat sampleFormat) -> std::vector<int16_t>;

}

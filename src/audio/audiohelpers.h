#pragma once

#include "audioformat.h"

#include <cstdint>
#include <vector>

namespace AudioHelpers
{
    /// @brief Convert audio data to int16_t
    /// @param samples Audio samples
    /// @param sampleFormat Sample format
    /// @return int16_t sample data
    auto toSigned16(const std::vector<uint8_t> &samples, Audio::SampleFormat sampleFormat) -> std::vector<int16_t>;

    /// @brief Create a sample buffer for a specific audio format
    /// @param sampleFormat
    /// @return Sample buffer for sample format
    auto createSampleBuffer(Audio::SampleFormat sampleFormat) -> Audio::SampleData;

    /// @brief Get raw size of sample data in bytes
    /// @return Raw size of sample data in bytes
    auto rawDataSize(const Audio::SampleData &samples) -> uint32_t;

    /// @brief Convert planar sample data to interleaved, raw byte buffer
    /// @param samples Planar sample data (e.g. L0 L1 ... R0 R1 ...)
    /// @param nrOfChannels Number of channels in sample data
    /// @return Interleaved sample data (e.g. L0 R0 L1 R1 ...) as raw byte buffer
    auto toRawInterleavedData(const Audio::SampleData &samples, uint32_t nrOfChannels) -> std::vector<uint8_t>;
}

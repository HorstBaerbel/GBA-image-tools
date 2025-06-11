#pragma once

#include "audioformat.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace Audio
{

    /// @brief Raw / compressed audio data
    using AudioData = std::variant<std::vector<int8_t>, std::vector<uint8_t>, std::vector<int16_t>, std::vector<uint16_t>, std::vector<float>>;

    /// @brief Information about current / final audio data before compression / conversion to raw
    struct FrameInfo
    {
        uint32_t sampleRateHz = 0;
        uint32_t channels = 0;
        SampleFormat sampleFormat = SampleFormat::Unknown;
        bool compressed = false;      // Flag if raw or compressed data is stored
        uint32_t maxMemoryNeeded = 0; // Max. intermediate memory needed to process the audio. 0 if it can be directly written to destination (single processing stage)
    };

    /// @brief Stores data for an audio frame
    struct Frame
    {
        uint32_t index = 0;   // Input frame index counter
        std::string fileName; // Input file name
        FrameInfo info;       // Frame information
        AudioData dataLeft;   // Raw / compressed data of left or mono channel
        AudioData dataRight;  // Raw / compressed data of right channel (or empty if mono audio)
    };

}

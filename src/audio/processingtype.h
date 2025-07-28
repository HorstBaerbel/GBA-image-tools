#pragma once

#include <cstdint>

namespace Audio
{

    /// @brief Type of processing to be done
    enum class ProcessingType : uint8_t
    {
        Uncompressed = 0,         // Verbatim data copy
        CompressLZ10 = 60,        // Compress audio data using LZ77 variant 10
        CompressRLE = 65,         // Compress audio data using run-length-encoding
        ConvertSamplesToRaw = 80, // Convert to final audio format and convert audio data to raw data
    };

    static constexpr uint8_t ProcessingTypeFinal = 128; // Marks the final processing step in an encoding sequence. Is ORed with Processing::Type

}

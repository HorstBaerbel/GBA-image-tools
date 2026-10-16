#pragma once

#include <cstdint>

namespace Audio
{
    /// @brief Type of processing to be done
    enum class ProcessingType : uint8_t
    {
        Uncompressed = 0,         // Verbatim data copy
        Resample = 10,            // Change audio channel format, sample format or sample rate
        Repackage = 20,           // Buffer audio and re-package for frame size
        CompressRLE = 60,         // Compress audio data using run-length-encoding
        CompressLZSS_10 = 61,     // Compress audio data using LZSS variant 10h
        CompressLZ4_40 = 64,      // Compress audio data using LZ4 variant 40h
        CompressRANS_50 = 65,     // Compress audio data using rANS variant 50h
        CompressADPCM = 70,       // Compress audio data as ADPCM samples
        ConvertSamplesToRaw = 80, // Convert audio data to raw data
        PadAudioData = 81,        // Fill up audio data with 0s to a multiple of N bytes
        Invalid = 255
    };
}

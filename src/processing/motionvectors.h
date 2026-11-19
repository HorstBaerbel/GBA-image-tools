#pragma once

#include "processing/bitstream.h"

#include <cstdint>

/// @brief Reads and write delta motion vectors using a lookup table:
/// - Contains the closest pixels in a radius of about +-12 pixels
/// - '0'+5 bit code: 32 closest vectors, range ~[-2,2]
/// - '1'+9 bit code: vectors 17-511, range ~[-12,12]
namespace MotionVectors
{
    static constexpr int32_t DeltaMinComponent = -12; // Absolute min. component value for x and y
    static constexpr int32_t DeltaMaxComponent = 12;  // Absolute max. component value for x and y

    static constexpr uint32_t DeltaShortCodePrefix = 0; // Prefix for short delta vector codes
    static constexpr uint32_t DeltaShortCodeLength = 5; // Length of short delta vector codes
    static constexpr uint32_t DeltaLongCodePrefix = 1;  // Prefix for long delta vector codes
    static constexpr uint32_t DeltaLongCodeLength = 9;  // Length of long delta vector codes

    struct DeltaVector
    {
        int8_t dx = 0;
        int8_t dy = 0;
    };

    /// @brief Decode table that maps an index to a delta vector
    extern const DeltaVector DeltaVectors[1 << DeltaLongCodeLength];

    /// Writes the a motion vector delta to the bit stream
    auto writeMotionDelta(Bitstream::Writer &bw, uint32_t index) -> void;

    /// Reads and decodes a motion vector delta from the bit stream
    auto readMotionDelta(Bitstream::Reader &br) -> DeltaVector;
}

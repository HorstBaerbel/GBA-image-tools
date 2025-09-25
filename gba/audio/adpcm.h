#pragma once

#include "sys/base.h"

#include <cstdint>

namespace Adpcm
{

    /// @brief Decode 4-bit ADPCM sample data
    /// @param data Pointer to 4-bit ADPCM data
    /// @param dst Pointer to output sample buffer(s)
    /// @param SAMPLE_BITS Final sample bit depth. ADPCM data will be downsampled to this depth
    template <unsigned SAMPLE_BITS>
    auto UnCompWrite32bit(const uint32_t *data, uint32_t dataSize, uint32_t *dst) -> void;

    /// @brief Get stored uncompressed size of sample data after decoding
    /// @param data Pointer to ADPCM data
    /// @param sampleBits Final sample bit depth
    template <unsigned SAMPLE_BITS>
    auto UnCompGetSize(const uint32_t *data) -> uint32_t;
}

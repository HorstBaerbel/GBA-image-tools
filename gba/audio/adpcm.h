#pragma once

#include "sys/base.h"

#include <cstdint>

namespace ADPCM
{

    /// @brief Decode ADPCM sample data
    /// @param data Pointer to ADPCM data
    /// @param dst Pointer to output sample buffer(s)
    /// @param sampleBits Final sample bit depth. ADPCM data will be downsampled to this depth
    void UnCompWrite32bit(const uint32_t *data, uint32_t *dst, uint32_t sampleBits);

    /// @brief Get stored uncompressed size of sample data after decoding
    /// @param data Pointer to ADPCM data
    /// @param sampleBits Final sample bit depth
    uint32_t UnCompGetSize(const uint32_t *data, uint32_t sampleBits);
}

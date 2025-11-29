#pragma once

#include "sys/base.h"

#include <cstdint>

/// @brief Decode 4-bit ADPCM sample data and truncate/dither to 8bit. Written in ARMv4 assembler
/// @param data Pointer to 4-bit ADPCM data
/// @param dst Pointer to output sample buffer(s)
extern "C" auto Adpcm_UnCompWrite32bit_8bit(const uint32_t *data, uint32_t dataSize, uint32_t *dst) -> void;

/// @brief Get stored uncompressed size of sample data after decoding and truncating to 8-bit. Written in ARMv4 assembler
/// @param data Pointer to ADPCM data
/// @note Only sample depths of (8, 16, 24, 32) are supported!
extern "C" auto Adpcm_UnCompGetSize_8bit(const uint32_t *data) -> uint32_t;

namespace Adpcm
{
    /// @brief Decode 4-bit ADPCM sample data
    /// @param data Pointer to 4-bit ADPCM data
    /// @param dst Pointer to output sample buffer(s)
    auto UnCompWrite32bit_8bit(const uint32_t *data, uint32_t dataSize, uint32_t *dst) -> void;

    /// @brief Get stored uncompressed size of sample data after decoding
    /// @param data Pointer to ADPCM data
    auto UnCompGetSize_8bit(const uint32_t *data) -> uint32_t;
}

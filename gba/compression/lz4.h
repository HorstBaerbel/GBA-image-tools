#pragma once

#include "sys/base.h"

#include <cstdint>

/// @brief Decompress LZ4 variant 40h, writing 8 bit at a time. Don't use for VRAM. Written in ARMv4 assembler
/// @param data Pointer to LZ4-compressed data
/// @param dst Pointer to output buffer
// extern "C" auto LZ4_UnCompWrite8bit(const uint32_t *data, uint32_t *dst) -> void;

namespace Compression
{
    /// @brief Decompress LZ4 variant 40h, writing 8 bit at a time. Don't use for VRAM
    /// @param data Pointer to LZ4-compressed data
    /// @param dst Pointer to output buffer
    auto LZ4UnCompWrite8bit(const uint32_t *data, uint32_t *dst) -> void;
}

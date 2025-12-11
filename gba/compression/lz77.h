#pragma once

#include "sys/base.h"

namespace Compression
{
    /// @brief Decompress LZSS GBA variant 10h, writing 8 bit at a time. Don't use for VRAM
    extern "C" void LZ77UnCompWrite8bit(const void *source, void *destination);

    /// @brief Decompress LZSS GBA variant 10h, writing 16 bit at a time. Safe for VRAM
    extern "C" void LZ77UnCompWrite16bit(const void *source, void *destination);

    /// @brief Get the size of the uncompressed data of BIOS-compatible data
    extern "C" unsigned int BIOSUnCompGetSize(const void *source);
}

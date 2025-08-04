#pragma once

#include <cstdint>

namespace DXTV
{

    /// @brief Decompress a 4x4 block of DXT data
    /// @param dataPtr16 Pointer to the compressed DXT data
    /// @param currPtr32 Pointer to the current output buffer
    /// @param prevPtr32 Pointer to the previous output buffer
    /// @param LineStride Line stride of the output buffer in bytes
    extern "C" auto DecodeBlock4x4(const uint16_t *dataPtr16, uint32_t *currPtr32, const uint32_t LineStride, const uint32_t *prevPtr32) -> const uint16_t *;

    /// @brief Decompress a 8x8 block of DXT data
    /// @param dataPtr16 Pointer to the compressed DXT data
    /// @param currPtr16 Pointer to the current output buffer
    /// @param LineStride Line stride of the output buffer in bytes
    extern "C" void UnDxtBlock8x8(const uint16_t *dataPtr16, uint16_t *currPtr16, const uint32_t LineStride);

    /// @brief Copy a 8x8 block from src to curr
    /// @param srcPtr16 Source pointer to the 8x8 block. Might be aligned or unaligned.
    /// @param currPtr32 Destination pointer to the 8x8 block. Always aligned.
    /// @param LineStride Line stride of the source buffer in bytes
    extern "C" void CopyBlock8x8(const uint16_t *srcPtr16, uint32_t *currPtr32, const uint32_t LineStride);

}

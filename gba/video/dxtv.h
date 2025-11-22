#pragma once

#include "sys/base.h"

#include <cstdint>

namespace Dxtv
{

    /// @brief Decompress image from DXTV format
    /// @param data Compressed image data in DXTV format
    /// @param dst Destination buffer. Must be able to hold a full decompressed image
    /// @param prevSrc Previous image to copy motion-compensated blocks from
    /// @param prevSrcLineStride Line stride in bytes for previous source image (e.g. 480 for VRAM mode 3)
    /// @param width Image width
    /// @param height Image height
    void UnCompWrite16bit(const uint32_t *data, uint32_t *dst, const uint32_t *prevSrc, const uint32_t prevSrcLineStride, uint32_t width, uint32_t height);

    /// @brief Get stored uncompressed size of image data after decoding
    /// @param data Pointer to DXTV data
    uint32_t UnCompGetSize(const uint32_t *data);
}

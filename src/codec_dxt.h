#pragma once

#include <cstdint>
#include <vector>

class DXT
{
public:
    /// @brief Compress image data to format similar to DXT1. See: https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format
    /// Differences:
    /// - Colors will be stored as RGB555 only
    static auto encodeDXTG(const std::vector<uint16_t> &image, uint32_t width, uint32_t height) -> std::vector<uint8_t>;

    /// @brief Decompress from DXTG format.
    static auto decodeDXTG(const std::vector<uint8_t> &data, uint32_t width, uint32_t height) -> std::vector<uint8_t>;

private:
    static std::vector<uint8_t> encodeBlockDXTG(const uint16_t *start, uint32_t pixelsPerScanline, const std::vector<std::vector<uint8_t>> &distanceSqrMap);

    static std::vector<std::vector<uint8_t>> RGB555DistanceSqrCache;
};

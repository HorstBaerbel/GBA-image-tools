#pragma once

#include "blockview.h"
#include "codebook.h"
#include "color/xrgb8888.h"
#include "statistics/statistics.h"

#include <cstdint>
#include <vector>

class DXTV
{
public:
    static constexpr uint32_t MAX_BLOCK_DIM = 8; // Maximum block size is 8x8 pixels

    static constexpr uint16_t FRAME_IS_PFRAME = 0x80; // 0 for B-frames / key frames with only intra-frame compression, 1 for P-frames with inter-frame compression ("predicted frame")
    static constexpr uint16_t FRAME_KEEP = 0x40;      // 1 for frames that are considered a direct copy of the previous frame and can be kept

    static constexpr bool BLOCK_NO_SPLIT = false;          // The block is a full block
    static constexpr bool BLOCK_IS_SPLIT = true;           // The block is split into smaller sub-blocks
    static constexpr uint16_t BLOCK_IS_DXT = 0;            // The block is a verbatim DXT block
    static constexpr uint16_t BLOCK_IS_REF = (1 << 15);    // The block is a motion-compensated block from the current or previous frame
    static constexpr uint16_t BLOCK_FROM_CURR = (0 << 14); // The reference block is from from the current frame
    static constexpr uint16_t BLOCK_FROM_PREV = (1 << 14); // The reference block is from from the previous frame

    static constexpr uint32_t BLOCK_MOTION_BITS = 6;                                         // Bits available for pixel motion
    static constexpr uint16_t BLOCK_MOTION_MASK = (uint16_t(1) << BLOCK_MOTION_BITS) - 1;    // Block x pixel motion mask
    static constexpr uint16_t BLOCK_MOTION_Y_SHIFT = BLOCK_MOTION_BITS;                      // Block y pixel motion shift

    static constexpr std::pair<int32_t, int32_t> CurrMotionHOffset{-((1 << BLOCK_MOTION_BITS) / 2 - 1), (1 << BLOCK_MOTION_BITS) / 2}; // Block position search offsets for current frame for 8, 4
    static constexpr std::pair<int32_t, int32_t> CurrMotionVOffset{-((1 << BLOCK_MOTION_BITS) / 2 - 1), 0};                            // Block position search offsets for current frame for 8, 4
    static constexpr std::pair<int32_t, int32_t> PrevMotionHOffset{-((1 << BLOCK_MOTION_BITS) / 2 - 1), (1 << BLOCK_MOTION_BITS) / 2}; // Block position search offsets for previous frame for 8, 4
    static constexpr std::pair<int32_t, int32_t> PrevMotionVOffset{-((1 << BLOCK_MOTION_BITS) / 2 - 1), (1 << BLOCK_MOTION_BITS) / 2}; // Block position search offsets for previous frame for 8, 4

    using CodeBook8x8 = CodeBook<Color::XRGB8888, MAX_BLOCK_DIM>; // Code book for storing 8x8 RGB pixel blocks

    /// @brief Compress image block to DXTV format
    template <std::size_t BLOCK_DIM>
    static auto encodeBlock(CodeBook8x8 &currentCodeBook, const CodeBook8x8 &previousCodeBook, BlockView<Color::XRGB8888, bool, BLOCK_DIM> &block, float quality, const bool swapToBGR = false, Statistics::Frame::SPtr statistics = nullptr) -> std::pair<bool, std::vector<uint8_t>>;

    /// @brief Compress image data to format similar to DXT1. See: https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format
    // DXT1 compresses one 4x4 block to 2 bytes color0, 2 bytes color1 and 16*2 bit = 4 bytes index information
    /// Difference is that colors will be stored as RGB555 only
    /// @param keyframe If true B-frame will be output, else a P-frame
    /// @param quality Quality for block references and splitting of blocks. The higher, the better quality. Range [0,100]
    /// @return Returns (compressed data, decompressed frame)
    static auto encode(const std::vector<Color::XRGB8888> &image, const std::vector<Color::XRGB8888> &previousImage, uint32_t width, uint32_t height, bool keyFrame, float quality, const bool swapToBGR = false, Statistics::Frame::SPtr statistics = nullptr) -> std::pair<std::vector<uint8_t>, std::vector<Color::XRGB8888>>;

    /// @brief Decompress block from DXTV format
    template <std::size_t BLOCK_DIM>
    static auto decodeBlock(const uint16_t *data, XRGB8888 *currBlock, const XRGB8888 *prevBlock, uint32_t width, const bool swapToBGR) -> const uint16_t *;

    /// @brief Decompress from DXTV format
    static auto decode(const std::vector<uint8_t> &data, const std::vector<Color::XRGB8888> &previousImage, uint32_t width, uint32_t height, const bool swapToBGR = false) -> std::vector<Color::XRGB8888>;
};

#pragma once

#include "blockview.h"
#include "codebook.h"
#include "color/xrgb8888.h"
#include "if/dxtv_constants.h"
#include "statistics/statistics.h"

#include <cstdint>
#include <vector>

namespace Video
{

    class Dxtv
    {
    public:
        static constexpr std::pair<int32_t, int32_t> CurrMotionHOffset{-((1 << DxtvConstants::BLOCK_MOTION_BITS) / 2 - 1), (1 << DxtvConstants::BLOCK_MOTION_BITS) / 2}; // Block position search offsets for current frame for 8, 4
        static constexpr std::pair<int32_t, int32_t> CurrMotionVOffset{-((1 << DxtvConstants::BLOCK_MOTION_BITS) / 2 - 1), 0};                                           // Block position search offsets for current frame for 8, 4
        static constexpr std::pair<int32_t, int32_t> PrevMotionHOffset{-((1 << DxtvConstants::BLOCK_MOTION_BITS) / 2 - 1), (1 << DxtvConstants::BLOCK_MOTION_BITS) / 2}; // Block position search offsets for previous frame for 8, 4
        static constexpr std::pair<int32_t, int32_t> PrevMotionVOffset{-((1 << DxtvConstants::BLOCK_MOTION_BITS) / 2 - 1), (1 << DxtvConstants::BLOCK_MOTION_BITS) / 2}; // Block position search offsets for previous frame for 8, 4

        using CodeBook8x8 = CodeBook<Color::XRGB8888, DxtvConstants::BLOCK_MAX_DIM>; // Code book for storing 8x8 RGB pixel blocks

        /// @brief Compress image block to DXTV format
        template <std::size_t BLOCK_DIM>
        static auto encodeBlock(CodeBook8x8 &currentCodeBook, const CodeBook8x8 &previousCodeBook, BlockView<Color::XRGB8888, bool, BLOCK_DIM> &block, float quality, const bool swapToBGR = false, Statistics::Frame::SPtr statistics = nullptr) -> std::pair<bool, std::vector<uint8_t>>;

        /// @brief Compress image to format similar to DXT1 (https://www.khronos.org/opengl/wiki/S3_Texture_Compression#DXT1_Format) while also using motion-compensation.
        /// The frame and block format can be found in src/if/dxtv_constants.h
        /// @param image Input image to compress
        /// @param previousImage Previous image to detect motion-compensated blocks
        /// @param width Image width. Must be a multiple of 8!
        /// @param height Image height. Must be a multiple of 8!
        /// @param quality Quality for block references and splitting of blocks. The higher, the better quality. Range [0,100]
        /// @param swapToBGR If true colors will have the blue and red color component swapped
        /// @param statistics Image processing statistics container
        /// @return Returns (compressed data, compressed/decompressed frame)
        static auto encode(const std::vector<Color::XRGB8888> &image, const std::vector<Color::XRGB8888> &previousImage, uint32_t width, uint32_t height, float quality, const bool swapToBGR = false, Statistics::Frame::SPtr statistics = nullptr) -> std::pair<std::vector<uint8_t>, std::vector<Color::XRGB8888>>;

        /// @brief Decompress block from DXTV format
        template <std::size_t BLOCK_DIM>
        static auto decodeBlock(const uint16_t *data, XRGB8888 *currBlock, const XRGB8888 *prevBlock, uint32_t width, const bool swapToBGR) -> const uint16_t *;

        /// @brief Decompress image from DXTV format
        /// @param data Compressed image data in DXTV format
        /// @param previousImage Previous image to copy motion-compensated blocks from
        /// @param width Image width. Must be a multiple of 8!
        /// @param height Image height. Must be a multiple of 8!
        /// @param swapToBGR If true colors will have the blue and red color component swapped
        static auto decode(const std::vector<uint8_t> &data, const std::vector<Color::XRGB8888> &previousImage, uint32_t width, uint32_t height, const bool swapToBGR = false) -> std::vector<Color::XRGB8888>;
    };

}
#pragma once

#include "media/vid2hio.h"

#include <cstdint>
#include <tuple>

namespace Vid2hDecoder
{

    /// @brief Decode video frame to scratchPad, possibly using a scratchPad as intermediate memory
    /// @param scratchPad32 Memory for decoding in bytes. Must be able to hold a full decoded frame AND intermediate memory. Must be aligned to 4 bytes!
    /// @param scratchPadSize8 Size of memory for decoding in bytes. Must be a multiple of 4 bytes!
    /// @param vramPtr8 Pointer to start of VRAM destination (for direct copy or getting previous frames)
    /// @param vramLineStride8 Line stride for one line in VRAM in bytes
    /// @param info Static video info
    /// @param frame Video frame to decode
    /// @return Returns pointer to decoded image and size of decoded samples in bytes
    auto DecodeVideo(uint32_t *scratchPad32, const uint32_t scratchPadSize8, uint8_t *vramPtr8, const uint32_t vramLineStride8, const IO::Vid2h::Info &info, const IO::Vid2h::Frame &frame) -> std::pair<const uint32_t *, uint32_t>;

    /// @brief Initialize audio decoders
    /// @param info Static video info
    /// @param dstRateHz Output sample rate in Hz. Must be > source sample rate
    auto DecodeAudioInit(const IO::Vid2h::Info &info, uint32_t dstRateHz) -> void;

    /// @brief Calculate maximum audio buffer size to hold one output audio frame if up- / downsampling
    /// @param info Static video info
    /// @param outputBits Output audio bit depth
    /// @param outputSampleRateHz Output audio sample rate
    /// @return Maximum buffer size needed to decode audio frame
    auto GetMaxAudioBufferSize(const IO::Vid2h::Info &info, uint32_t outputBits, uint32_t outputSampleRateHz) -> uint32_t;

    /// @brief Decode audio frame to scratchPad, possibly using a scratchPad as intermediate memory
    /// @param outputBuffer32 Final output audio buffer(s). Must be able to hold full decoded frame(s)
    /// @param scratchPad32 Intermediate memory for decoding in bytes. Must be able to hold a full decoded frame or intermediate memory. Must be aligned to 4 bytes!
    /// @param info Static video info
    /// @param frame Audio frame to decode
    /// @return Returns size of decoded samples in bytes
    auto DecodeAudio(uint8_t *outputBuffer8[2], uint32_t *scratchPad32, const IO::Vid2h::Info &info, const IO::Vid2h::Frame &frame) -> uint32_t;

    /// @brief Decode subtitles frame
    /// @param frame Subtitles frame to decode
    /// @return Returns (start time, end time, text length, text) or (0, 0, 0, nullptr) if decoding failed
    auto DecodeSubtitles(const IO::Vid2h::Frame &frame) -> std::tuple<int32_t, int32_t, const char *>;
}

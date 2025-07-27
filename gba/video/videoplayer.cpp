#include "videoplayer.h"

#include "memory/memory.h"
#include "sys/base.h"
#include "vid2hdecoder.h"

#include <gba_interrupt.h>
#include <gba_timers.h>

#define DEBUG_PLAYER
#ifdef DEBUG_PLAYER
#include "print/output.h"
#include "time.h"
#endif

namespace Media
{

    struct SampleBuffer
    {
        uint8_t *channel0 = nullptr; // Pointer to data for left / mono channel
        uint8_t *channel1 = nullptr; // Pointer to data for right channel (if stereo)
        uint32_t channelSize = 0;    // Size of data stored per channel
    } __attribute__((aligned(4), packed));

    IWRAM_DATA bool m_playing = false;
    IWRAM_DATA IO::Vid2h::Info m_mediaInfo;
    IWRAM_DATA IO::Vid2h::Frame m_mediaFrame;
    // audio
    IWRAM_DATA uint32_t *m_audioData = nullptr;
    IWRAM_DATA uint32_t m_audioDataSize = 0;
    IWRAM_DATA int32_t m_audioFrameIndex = 0;
    IWRAM_DATA int32_t m_audioFramesDecoded = 0;
    IWRAM_DATA SampleBuffer m_audioSampleBuffers[2];  // Buffers holding sample data
    IWRAM_DATA uint32_t m_audioSampleBufferIndex = 0; // Currently played buffer index
    // video
    IWRAM_DATA uint32_t *m_videoScratchPad = nullptr;
    IWRAM_DATA uint32_t m_videoScratchPadSize = 0;
    IWRAM_DATA int32_t m_videoColorMapFrameIndex = 0;
    IWRAM_DATA int32_t m_videoFrameIndex = 0;
    IWRAM_DATA int32_t m_videoFramesDecoded = 0;
    IWRAM_DATA const uint32_t *m_videoDecodedFrame = nullptr;
    IWRAM_DATA uint32_t m_videoDecodedFrameSize = 0;
    IWRAM_DATA uint32_t m_clearColor = 0;

#ifdef DEBUG_PLAYER
    IWRAM_DATA int32_t m_accFrameDecodeMs = 0;
    IWRAM_DATA int32_t m_maxFrameDecodeMs = 0;
    IWRAM_DATA int32_t m_nrOfFramesDecoded = 0;
    IWRAM_DATA int32_t m_accFrameBlitMs = 0;
    IWRAM_DATA int32_t m_maxFrameBlitMs = 0;
    IWRAM_DATA int32_t m_nrOfFramesBlit = 0;
#endif

    IWRAM_DATA volatile int32_t m_videoFramesRequested = 0;
    IWRAM_FUNC auto FrameRequest() -> void
    {
        ++m_videoFramesRequested;
    }

    auto Init(const uint32_t *videoSrc, uint32_t *videoScratchPad, uint32_t videoScratchPadSize, uint32_t *audioData, uint32_t audioDataSize) -> void
    {
        m_videoScratchPad = videoScratchPad;
        m_videoScratchPadSize = videoScratchPadSize;
        m_audioData = audioData;
        m_audioDataSize = audioDataSize;
        // read file header
        m_mediaInfo = IO::Vid2h::GetInfo(videoSrc);
        auto bytesPerPixel = (m_mediaInfo.videoBitsPerPixel + 7) / 8;
        m_videoDecodedFrameSize = m_mediaInfo.videoWidth * m_mediaInfo.videoHeight * bytesPerPixel;
        // set up audio buffers
        const auto audioData8 = reinterpret_cast<uint8_t *>(audioData);
        if (m_mediaInfo.audioChannels == 1)
        {
            m_audioSampleBuffers[0] = {audioData8 + 0 * audioDataSize / 2, nullptr, 0};
            m_audioSampleBuffers[1] = {audioData8 + 1 * audioDataSize / 2, nullptr, 0};
        }
        else if (m_mediaInfo.audioChannels == 2)
        {
            m_audioSampleBuffers[0] = {audioData8 + 0 * audioDataSize / 4, audioData8 + 1 * audioDataSize / 4, 0};
            m_audioSampleBuffers[1] = {audioData8 + 2 * audioDataSize / 4, audioData8 + 3 * audioDataSize / 4, 0};
        }
    }

    auto SetClearColor(uint16_t color) -> void
    {
        m_clearColor = static_cast<uint32_t>(color) << 16 | color;
    }

    auto GetInfo() -> const IO::Vid2h::Info &
    {
        return m_mediaInfo;
    }

    auto Play() -> void
    {
        if (!m_playing)
        {
            m_mediaFrame.index = -1;
            m_mediaFrame.header = nullptr;
            m_mediaFrame.data = nullptr;
            m_audioFrameIndex = -1;
            m_audioFramesDecoded = 0;
            m_videoFrameIndex = -1;
            m_videoFramesDecoded = 0;
            m_videoFramesRequested = 1;
            m_playing = true;
#ifdef DEBUG_PLAYER
            m_accFrameDecodeMs = 0;
            m_maxFrameDecodeMs = 0;
            m_nrOfFramesDecoded = 0;
            m_accFrameBlitMs = 0;
            m_maxFrameBlitMs = 0;
            m_nrOfFramesBlit = 0;
            Time::start();
#endif
            // fill audio buffers with silence
            Memory::memset32(m_audioData, 0, m_audioDataSize / 4);
            // fill background and scratch pad to clear color
            Memory::memset32((void *)VRAM, m_clearColor, m_mediaInfo.videoWidth * m_mediaInfo.videoHeight / 2);
            Memory::memset32(m_videoScratchPad, m_clearColor, m_videoScratchPadSize / 4);
            // set up timer to increase with frame interval
            irqSet(irqMASKS::IRQ_TIMER2, FrameRequest);
            irqEnable(irqMASKS::IRQ_TIMER2);
            // Timer interval = 1 / fps (where 65536 == 1s). frames/s are in 16:16 format
            REG_TM2CNT_L = 65536U - ((uint64_t(65536U) << 16) / m_mediaInfo.videoFrameRateHz);
            // Timer divider 2 == 256 -> 16*1024*1024 cycles/s / 256 = 65536/s
            REG_TM2CNT_H = TIMER_START | TIMER_IRQ | 2;
        }
    }

    auto Stop() -> void
    {
        if (m_playing)
        {
            REG_TM2CNT_H = 0;
            irqDisable(irqMASKS::IRQ_TIMER2);
            m_playing = false;
            m_videoFramesRequested = 0;
#ifdef DEBUG_PLAYER
            Debug::printf("Avg. decode: %f ms (max. %f ms)", m_accFrameDecodeMs / m_nrOfFramesDecoded, m_maxFrameDecodeMs);
            Debug::printf("Avg. blit: %f ms (max. %f ms)", m_accFrameBlitMs / m_nrOfFramesBlit, m_maxFrameBlitMs);
            Time::stop();
#endif
        }
    }

    IWRAM_FUNC auto HasMoreFrames() -> bool
    {
        return m_playing && IO::Vid2h::HasMoreFrames(m_mediaInfo, m_mediaFrame);
    }

    IWRAM_FUNC auto DecodeAndBlitFrame(uint32_t *dst) -> void
    {
        if (m_playing)
        {
            if (m_videoFramesDecoded < 1)
            {
                // read next frame from data
                m_mediaFrame = IO::Vid2h::GetNextFrame(m_mediaInfo, m_mediaFrame);
                if (m_mediaFrame.dataType == IO::FrameType::Pixels)
                {
                    m_videoFrameIndex = m_mediaFrame.index;
#ifdef DEBUG_PLAYER
                    auto startTime = Time::now();
#endif
                    // uncompress frame
                    m_videoDecodedFrame = DecodeVideo(m_videoScratchPad, m_videoScratchPadSize, m_mediaInfo, m_mediaFrame);
                    ++m_videoFramesDecoded;
#ifdef DEBUG_PLAYER
                    auto duration = Time::now() * 1000 - startTime * 1000;
                    m_accFrameDecodeMs += duration;
                    m_maxFrameDecodeMs = m_maxFrameDecodeMs < duration ? duration : m_maxFrameDecodeMs;
                    ++m_nrOfFramesDecoded;
#endif
                }
                else if (m_mediaFrame.dataType == IO::FrameType::Audio)
                {
                    m_audioFrameIndex = m_mediaFrame.index;
                    // copy audio data to sample buffer
                    auto &dstSampleBuffer = m_audioSampleBufferIndex == 0 ? m_audioSampleBuffers[1] : m_audioSampleBuffers[0];
                    if (m_mediaInfo.audioChannels == 1)
                    {
                        Memory::memcpy32(dstSampleBuffer.channel0, m_mediaFrame.data, m_mediaFrame.dataSize / 4);
                        dstSampleBuffer.channelSize = m_mediaFrame.dataSize;
                    }
                    else if (m_mediaInfo.audioChannels == 2)
                    {
                        const auto channelSize = m_mediaFrame.dataSize / 2;
                        auto srcData8 = reinterpret_cast<const uint8_t *>(m_mediaFrame.data);
                        Memory::memcpy32(dstSampleBuffer.channel0, srcData8, channelSize / 4);
                        Memory::memcpy32(dstSampleBuffer.channel1, srcData8 + channelSize, channelSize / 4);
                        dstSampleBuffer.channelSize = channelSize;
                    }
                    ++m_audioFramesDecoded;
                }
            }
            if (m_videoFramesRequested > 0)
            {
                --m_videoFramesRequested;
                if (m_videoFramesDecoded > 0)
                {
#ifdef DEBUG_PLAYER
                    auto startTime = Time::now();
#endif
                    // we're waiting for a frame and have one. blit it!
                    m_videoFramesDecoded = 0;
                    Memory::memcpy32(dst, m_videoDecodedFrame, m_videoDecodedFrameSize / 4);
#ifdef DEBUG_PLAYER
                    auto duration = Time::now() * 1000 - startTime * 1000;
                    m_accFrameBlitMs += duration;
                    m_maxFrameBlitMs = m_maxFrameBlitMs < duration ? duration : m_maxFrameBlitMs;
                    ++m_nrOfFramesBlit;
#endif
                }
                if (m_videoFramesRequested > 0)
                {
#ifdef DEBUG_PLAYER
                    Debug::printf("Skipping %d frame(s)", m_videoFramesRequested);
#endif
                    m_videoFramesRequested = 0;
                }
            }
        }
    }

}

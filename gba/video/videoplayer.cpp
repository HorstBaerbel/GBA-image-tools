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

    IWRAM_DATA uint32_t *m_scratchPad = nullptr;
    IWRAM_DATA uint32_t m_scratchPadSize = 0;
    IWRAM_DATA IO::Vid2h::Info m_videoInfo;
    IWRAM_DATA IO::Vid2h::Frame m_videoFrame;
    IWRAM_DATA bool m_playing = false;
    IWRAM_DATA const uint32_t *m_decodedFrame = nullptr;
    IWRAM_DATA uint32_t m_decodedFrameSize = 0;
    IWRAM_DATA int32_t m_framesDecoded = 0;
    IWRAM_DATA uint32_t m_clearColor = 0;

#ifdef DEBUG_PLAYER
    IWRAM_DATA int32_t m_accFrameDecodeMs = 0;
    IWRAM_DATA int32_t m_maxFrameDecodeMs = 0;
    IWRAM_DATA int32_t m_nrOfFramesDecoded = 0;
    IWRAM_DATA int32_t m_accFrameBlitMs = 0;
    IWRAM_DATA int32_t m_maxFrameBlitMs = 0;
    IWRAM_DATA int32_t m_nrOfFramesBlit = 0;
#endif

    IWRAM_DATA volatile int32_t m_framesRequested = 0;
    IWRAM_FUNC auto FrameRequest() -> void
    {
        ++m_framesRequested;
    }

    auto Init(const uint32_t *videoSrc, uint32_t *scratchPad, uint32_t scratchPadSize) -> void
    {
        m_scratchPad = scratchPad;
        m_scratchPadSize = scratchPadSize;
        // read file header
        m_videoInfo = IO::Vid2h::GetInfo(videoSrc);
        auto bytesPerPixel = (m_videoInfo.videoBitsPerPixel + 7) / 8;
        m_decodedFrameSize = m_videoInfo.videoWidth * m_videoInfo.videoHeight * bytesPerPixel;
    }

    auto SetClearColor(uint16_t color) -> void
    {
        m_clearColor = static_cast<uint32_t>(color) << 16 | color;
    }

    auto GetInfo() -> const IO::Vid2h::Info &
    {
        return m_videoInfo;
    }

    auto Play() -> void
    {
        if (!m_playing)
        {
            m_videoFrame.index = -1;
            m_videoFrame.header = nullptr;
            m_videoFrame.data = nullptr;
            m_playing = true;
            m_framesDecoded = 0;
            m_framesRequested = 1;
#ifdef DEBUG_PLAYER
            m_accFrameDecodeMs = 0;
            m_maxFrameDecodeMs = 0;
            m_nrOfFramesDecoded = 0;
            m_accFrameBlitMs = 0;
            m_maxFrameBlitMs = 0;
            m_nrOfFramesBlit = 0;
            Time::start();
#endif
            // fill background and scratch pad to clear color
            Memory::memset32((void *)VRAM, m_clearColor, m_videoInfo.videoWidth * m_videoInfo.videoHeight / 2);
            Memory::memset32(m_scratchPad, m_clearColor, m_scratchPadSize / 4);
            // set up timer to increase with frame interval
            irqSet(irqMASKS::IRQ_TIMER2, FrameRequest);
            irqEnable(irqMASKS::IRQ_TIMER2);
            // Timer interval = 1 / fps (where 65536 == 1s). frames/s are in 16:16 format
            REG_TM2CNT_L = 65536U - ((uint64_t(65536U) << 16) / m_videoInfo.videoFrameRateHz);
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
            m_framesRequested = 0;
#ifdef DEBUG_PLAYER
            Debug::printf("Avg. decode: %f ms (max. %f ms)", m_accFrameDecodeMs / m_nrOfFramesDecoded, m_maxFrameDecodeMs);
            Debug::printf("Avg. blit: %f ms (max. %f ms)", m_accFrameBlitMs / m_nrOfFramesBlit, m_maxFrameBlitMs);
            Time::stop();
#endif
        }
    }

    IWRAM_FUNC auto HasMoreFrames() -> bool
    {
        return m_playing && m_videoFrame.index < static_cast<int32_t>(m_videoInfo.videoNrOfFrames - 1);
    }

    IWRAM_FUNC auto DecodeAndBlitFrame(uint32_t *dst) -> void
    {
        if (m_playing)
        {
            if (m_framesDecoded < 1)
            {
                ++m_framesDecoded;
#ifdef DEBUG_PLAYER
                auto startTime = Time::now();
#endif
                // read next frame from data
                m_videoFrame = GetNextFrame(m_videoInfo, m_videoFrame);
                // uncompress frame
                m_decodedFrame = Decode(m_scratchPad, m_scratchPadSize, m_videoInfo, m_videoFrame);
#ifdef DEBUG_PLAYER
                auto duration = Time::now() * 1000 - startTime * 1000;
                m_accFrameDecodeMs += duration;
                m_maxFrameDecodeMs = m_maxFrameDecodeMs < duration ? duration : m_maxFrameDecodeMs;
                ++m_nrOfFramesDecoded;
#endif
            }
            if (m_framesRequested > 0)
            {
                --m_framesRequested;
                if (m_framesDecoded > 0)
                {
#ifdef DEBUG_PLAYER
                    auto startTime = Time::now();
#endif
                    // we're waiting for a frame and have one. blit it!
                    m_framesDecoded = 0;
                    Memory::memcpy32(dst, m_decodedFrame, m_decodedFrameSize / 4);
#ifdef DEBUG_PLAYER
                    auto duration = Time::now() * 1000 - startTime * 1000;
                    m_accFrameBlitMs += duration;
                    m_maxFrameBlitMs = m_maxFrameBlitMs < duration ? duration : m_maxFrameBlitMs;
                    ++m_nrOfFramesBlit;
#endif
                }
                if (m_framesRequested > 0)
                {
#ifdef DEBUG_PLAYER
                    Debug::printf("Skipping %d frame(s)", m_framesRequested);
#endif
                    m_framesRequested = 0;
                }
            }
        }
    }

}

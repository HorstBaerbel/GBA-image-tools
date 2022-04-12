#include "videoplayer.h"

#include "memory/memory.h"
#include "sys/base.h"
#include "videodecoder.h"
#include "videoreader.h"

#include <gba_interrupt.h>
#include <gba_timers.h>

namespace Video
{

    uint32_t *m_scratchPad = nullptr;
    uint32_t m_scratchPadSize = 0;
    Video::Info m_videoInfo{};
    Video::Frame m_videoFrame{};
    const uint32_t *m_decodedFrame = nullptr;
    uint32_t m_decodedFrameSize = 0;
    bool m_playing = false;

    IWRAM_DATA volatile bool m_frameRequested = true;
    IWRAM_FUNC auto frameRequest() -> void
    {
        m_frameRequested = true;
    }

    auto init(const uint32_t *videoSrc, uint32_t *scratchPad, uint32_t scratchPadSize) -> void
    {
        m_scratchPad = scratchPad;
        m_scratchPadSize = scratchPadSize;
        // read file header
        m_videoInfo = Video::GetInfo(videoSrc);
        auto bytesPerPixel = (m_videoInfo.bitsPerPixel + 7) / 8;
        m_decodedFrameSize = m_videoInfo.width * m_videoInfo.height * bytesPerPixel;
    }

    auto getInfo() -> const Video::Info &
    {
        return m_videoInfo;
    }

    auto play() -> void
    {
        if (!m_playing)
        {
            m_playing = true;
            m_frameRequested = true;
            m_videoFrame = Video::Frame{};
            // set up timer to increase with frame interval
            irqSet(irqMASKS::IRQ_TIMER3, frameRequest);
            irqEnable(irqMASKS::IRQ_TIMER3);
            // Timer interval = 1 / fps (where 65536 == 1s)
            REG_TM3CNT_L = 65536 - (65536 / m_videoInfo.fps);
            // Timer divider 2 == 256 -> 16*1024*1024 cycles/s / 256 = 65536/s
            REG_TM3CNT_H = TIMER_START | TIMER_IRQ | 2;
        }
    }

    auto stop() -> void
    {
        if (m_playing)
        {
            REG_TM3CNT_H = 0;
            irqDisable(irqMASKS::IRQ_TIMER3);
            m_playing = false;
            m_frameRequested = false;
        }
    }

    auto hasMoreFrames() -> bool
    {
        return m_playing && m_videoFrame.index < (m_videoInfo.nrOfFrames - 1);
    }

    auto decodeFrame() -> void
    {
        if (m_playing && m_frameRequested)
        {
            m_frameRequested = false;
            // read next frame from data
            m_videoFrame = GetNextFrame(m_videoInfo, m_videoFrame);
            // uncompress frame
            m_decodedFrame = decode(m_scratchPad, m_scratchPadSize, m_videoInfo, m_videoFrame);
        }
    }

    auto blitFrameTo(uint32_t *dst) -> void
    {
        Memory::memcpy32(dst, m_decodedFrame, m_decodedFrameSize / 4);
    }

}
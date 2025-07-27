#include "videoplayer.h"

#include "memory/memory.h"
#include "sys/base.h"
#include "vid2hdecoder.h"

#include <gba_dma.h>
#include <gba_interrupt.h>
#include <gba_sound.h>
#include <gba_timers.h>

// #define DEBUG_PLAYER
#ifdef DEBUG_PLAYER
#include "print/output.h"
#include "time.h"
#endif

namespace Media
{

    // general
    IWRAM_DATA bool m_playing = false;
    IWRAM_DATA IO::Vid2h::Info m_mediaInfo;
    IWRAM_DATA IO::Vid2h::Frame m_mediaFrame;

    // audio
    struct SampleBuffer
    {

    } __attribute__((aligned(4), packed));

    uint32_t *m_audioData = nullptr;
    uint32_t m_audioDataSize = 0;
    IWRAM_DATA int32_t m_audioFrameIndex = -1;
    IWRAM_DATA int32_t m_audioFramesDecoded = 0;
    IWRAM_DATA uint32_t *m_audioPlaySampleBuffer = nullptr; // Pointer to planar data for left / mono channel and right channel currently playing
    IWRAM_DATA uint32_t *m_audioBackSampleBuffer = nullptr; // Pointer to planar data for left / mono channel and right channel currently decoded
    IWRAM_DATA uint32_t m_audioSampleBufferChannelSize = 0; // Size of data stored per channel == number of samples per channel

    // video
    uint32_t *m_videoScratchPad = nullptr;
    uint32_t m_videoScratchPadSize = 0;
    uint32_t m_clearColor = 0;
    IWRAM_DATA int32_t m_videoColorMapFrameIndex = 0;
    IWRAM_DATA int32_t m_videoFrameIndex = -1;
    IWRAM_DATA int32_t m_videoFramesDecoded = 0;
    IWRAM_DATA int32_t m_videoFramesRequested = 0;
    IWRAM_DATA const uint32_t *m_videoDecodedFrame = nullptr;
    IWRAM_DATA uint32_t m_videoDecodedFrameSize = 0;

#ifdef DEBUG_PLAYER
    IWRAM_DATA int32_t m_accFrameDecodeMs = 0;
    IWRAM_DATA int32_t m_maxFrameDecodeMs = 0;
    IWRAM_DATA int32_t m_nrOfFramesDecoded = 0;
    IWRAM_DATA int32_t m_accFrameBlitMs = 0;
    IWRAM_DATA int32_t m_maxFrameBlitMs = 0;
    IWRAM_DATA int32_t m_nrOfFramesBlit = 0;
#endif

    IWRAM_FUNC auto AudioSampleBufferRequest() -> void
    {
        if (m_playing && m_audioFrameIndex < m_mediaInfo.audioNrOfFrames)
        {
            // still playing back. stop DMA channels
            REG_DMA1CNT &= ~DMA_ENABLE;
            if (m_mediaInfo.audioChannels == 2)
            {
                REG_DMA2CNT &= ~DMA_ENABLE;
            }
            // swap sample buffers
            auto tmp = m_audioBackSampleBuffer;
            m_audioBackSampleBuffer = m_audioPlaySampleBuffer;
            m_audioPlaySampleBuffer = tmp;
            // the FIFO(s) are empty. write first word to FIFO(s)
            REG_FIFO_A = *m_audioPlaySampleBuffer;
            if (m_mediaInfo.audioChannels == 2)
            {
                REG_FIFO_B = *(m_audioPlaySampleBuffer + m_audioSampleBufferChannelSize / 4);
            }
            // calculate buffer start after first word
            const auto sampleBufferStart = reinterpret_cast<uint32_t>(m_audioPlaySampleBuffer) + 4;
            // set DMA channels to read from new buffer and restart DMA
            REG_DMA1SAD = sampleBufferStart;
            REG_DMA1CNT |= DMA_ENABLE;
            if (m_mediaInfo.audioChannels == 2)
            {
                REG_DMA2SAD = sampleBufferStart + m_audioSampleBufferChannelSize;
                REG_DMA2CNT |= DMA_ENABLE;
            }
            // set timer 1 count to number of words in new buffer (minus first word)
            REG_TM1CNT_L = m_audioSampleBufferChannelSize - 4;
            // start both timers
            REG_TM0CNT_H |= TIMER_START;
            REG_TM1CNT_H |= TIMER_START;
            // enable sound
            REG_SOUNDCNT_X = SOUND3_PLAY;
        }
        else
        {
            // this was the last frame. turn off sounds
            REG_SOUNDCNT_X = 0;
        }
    }

    IWRAM_FUNC auto VideoFrameRequest() -> void
    {
        if (m_playing && m_videoFrameIndex < m_mediaInfo.videoNrOfFrames)
        {
            // still playing back. request video frame to be displayed
            ++m_videoFramesRequested;
        }
    }

    auto Init(const uint32_t *videoSrc, uint32_t *videoScratchPad, uint32_t videoScratchPadSize, uint32_t *audioData, uint32_t audioDataSize) -> void
    {
        // read file header
        m_mediaInfo = IO::Vid2h::GetInfo(videoSrc);
        if (m_mediaInfo.contentType & IO::FileType::Video)
        {
            // set up video buffers
            m_videoScratchPad = videoScratchPad;
            m_videoScratchPadSize = videoScratchPadSize;
            auto bytesPerPixel = (m_mediaInfo.videoBitsPerPixel + 7) / 8;
            m_videoDecodedFrameSize = m_mediaInfo.videoWidth * m_mediaInfo.videoHeight * bytesPerPixel;
        }
        if (m_mediaInfo.contentType & IO::FileType::Audio)
        {
            // set up audio buffers
            m_audioData = audioData;
            m_audioDataSize = audioDataSize;
            m_audioPlaySampleBuffer = audioData;
            m_audioBackSampleBuffer = audioData + audioDataSize / (2 * 4);
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
            m_videoColorMapFrameIndex = -1;
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
            if (m_mediaInfo.contentType & IO::FileType::Audio)
            {
                // disable all sounds for now
                REG_SOUNDCNT_X = 0;
                // fill audio buffers with silence
                Memory::memset32(m_audioData, 0, m_audioDataSize / 4);
                // enable DMA 1 to copy words to sound FIFO A
                // REG_DMA1SAD = reinterpret_cast<uint32_t>(m_audioBackSampleBuffer->channel0);
                REG_DMA1DAD = reinterpret_cast<uint32_t>(&REG_FIFO_A);                        // write to sound FIFO A
                REG_DMA1CNT = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT | DMA32 | DMA_SPECIAL; // start DMA later
                if (m_mediaInfo.audioChannels == 1)
                {
                    // set DMA channel A to output to left and right at 100% and reset FIFO. by default timer 0 is used for both channels
                    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
                }
                else if (m_mediaInfo.audioChannels == 2)
                {
                    // enable DMA 2 to copy words to sound FIFO B
                    // REG_DMA2SAD = reinterpret_cast<uint32_t>(m_audioBackSampleBuffer->channel1);
                    REG_DMA2DAD = reinterpret_cast<uint32_t>(&REG_FIFO_B);                        // write to sound FIFO B
                    REG_DMA2CNT = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT | DMA32 | DMA_SPECIAL; // start DMA later
                    // set DMA channel A to output to left at 100% and reset FIFO
                    // set DMA channel B to output to right at 100% and reset FIFO
                    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_RESET_FIFO | SNDB_VOL_100 | SNDB_R_ENABLE | SNDB_RESET_FIFO;
                }
                // set up timer 0 to increase with sample rate = 1 / sample rate (where 16777216 == 1s) and set divider to 0 == 1/1
                REG_TM0CNT_L = 65536U - (uint64_t(16777216) / m_mediaInfo.audioSampleRateHz);
                REG_TM0CNT_H = 0; // start timer later
                // set timer 1 to cascade from timer 0, issue IRQ to swap sample buffers and set divider to 0 == 1/1
                irqSet(irqMASKS::IRQ_TIMER1, AudioSampleBufferRequest);
                irqEnable(irqMASKS::IRQ_TIMER1);
                REG_TM1CNT_L = 0;                           // set up number of samples in buffer later
                REG_TM1CNT_H = TIMER_COUNT | TIMER_IRQ | 0; // start timer later
            }
            if (m_mediaInfo.contentType & IO::FileType::Video)
            {
                // fill background and scratch pad to clear color
                Memory::memset32((void *)VRAM, m_clearColor, m_mediaInfo.videoWidth * m_mediaInfo.videoHeight / 2);
                Memory::memset32(m_videoScratchPad, m_clearColor, m_videoScratchPadSize / 4);
                // set up timer 2 to increase with video frame interval = 1 / fps (where 65536 == 1s). frames/s are in 16:16 format
                // set timer 2 divider to 2 == 1/256 -> 16*1024*1024 cycles/s / 256 = 65536/s
                irqSet(irqMASKS::IRQ_TIMER2, VideoFrameRequest);
                irqEnable(irqMASKS::IRQ_TIMER2);
                REG_TM2CNT_L = 65536U - ((uint64_t(65536U) << 16) / m_mediaInfo.videoFrameRateHz);
                REG_TM2CNT_H = TIMER_START | TIMER_IRQ | 2;
            }
        }
    }

    auto Stop() -> void
    {
        if (m_playing)
        {
            // disable sound for now
            REG_SOUNDCNT_X = 0;
            // disable audio timers
            REG_TM0CNT_H = 0;
            REG_TM1CNT_H = 0;
            irqDisable(irqMASKS::IRQ_TIMER1);
            // disable audio DMAs
            REG_DMA1CNT = 0;
            if (m_mediaInfo.audioChannels == 2)
            {
                REG_DMA2CNT = 0;
            }
            // disable video timer
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
                    ++m_videoFrameIndex;
#ifdef DEBUG_PLAYER
                    auto startTime = Time::now();
#endif
                    // uncompress video frame
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
                    ++m_audioFrameIndex;
                    // uncompress audio frame
                    auto [audioDecodedFrame, audioDecodedFrameSize] = DecodeAudio(m_audioBackSampleBuffer, m_audioDataSize / 2, m_mediaInfo, m_mediaFrame);
                    m_audioSampleBufferChannelSize = audioDecodedFrameSize;
                    ++m_audioFramesDecoded;
                    // if this is the first frame, start playback
                    if (m_audioFrameIndex == 0)
                    {
                        AudioSampleBufferRequest();
                    }
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

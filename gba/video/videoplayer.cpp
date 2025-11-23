#include "videoplayer.h"

#include "memory/memory.h"
#include "sys/base.h"
#include "vid2hdecoder.h"

#include <gba_dma.h>
#include <gba_interrupt.h>
#include <gba_sound.h>
#include <gba_timers.h>

#define DEBUG_PLAYER
#ifdef DEBUG_PLAYER
#include "print/output.h"
#include "time.h"
#endif

// BBB
// Video avg. decode: 23.28 ms (max. 35.16 ms) C++
// Video avg. decode: 17.13 ms (max. 26.37 ms) ASM
// Audio avg. decode: 11.38 ms (max. 11.72 ms) C++
// Audio avg. decode: 6.92 ms (max. 7.81 ms) ASM

// AF
// Video avg. decode: 26.61 ms (max. 34.18 ms) C++
// Video avg. decode: 19.67 ms (max. 26.37 ms) ASM

namespace Media
{
    // general
    IWRAM_DATA bool m_playing = false;
    IWRAM_DATA IO::Vid2h::Info m_mediaInfo;
    IWRAM_DATA IO::Vid2h::Frame m_mediaFrame;

    // audio
    IWRAM_DATA IO::Vid2h::Frame m_queuedAudioFrame{};
    IWRAM_DATA volatile int16_t m_audioFramesDecoded = 0;   // Number of audio frames decoded in m_audioBackSampleBuffer
    IWRAM_DATA volatile int16_t m_audioFramesRequested = 0; // Number of audio frames requested by AudioBufferRequest()
    IWRAM_DATA uint32_t *m_audioPlayBuffer = nullptr;       // Pointer to planar data for left / mono channel and right channel currently playing
    IWRAM_DATA uint32_t *m_audioBackBuffer = nullptr;       // Pointer to planar data for left / mono channel and right channel currently decoding
    IWRAM_DATA uint16_t m_audioBufferSize = 0;              // Size of one audio buffer
    IWRAM_DATA uint16_t m_audioPlayBufferChannelSize = 0;   // Size of data stored per channel == number of samples per channel of buffer currently playing
    IWRAM_DATA uint16_t m_audioBackBufferChannelSize = 0;   // Size of data stored per channel == number of samples per channel of buffer currently decoding

    // video
    IWRAM_DATA uint32_t *m_videoScratchPad = nullptr;
    IWRAM_DATA uint32_t m_videoScratchPadSize = 0;
    IWRAM_DATA uint16_t m_vramStride = 0;
    IWRAM_DATA uint16_t m_clearColor = 0;
    IWRAM_DATA IO::Vid2h::Frame m_queuedColorMapFrame{};
    IWRAM_DATA IO::Vid2h::Frame m_queuedVideoFrame{};
    IWRAM_DATA volatile int16_t m_videoFramesDecoded = 0;     // Number of video frames decoded in m_videoDecodedFrame
    IWRAM_DATA volatile int16_t m_videoFramesRequested = 0;   // Number of video frames requested by VideoFrameRequest()
    IWRAM_DATA const uint32_t *m_videoDecodedFrame = nullptr; // Pointer to decoded video frame
    IWRAM_DATA uint32_t m_videoDecodedFrameSize = 0;          // Size of decoded video frame

#ifdef DEBUG_PLAYER
    struct FrameStatistics
    {
        int32_t m_accDecodedMs = 0; // Accumulated time decoding frames
        int32_t m_maxDecodedMs = 0; // Max. time decoding frames
        int32_t m_nrDecoded = 0;    // Number of frames decoded so far
        int32_t m_accCopiedMs = 0;  // Accumulated time displaying / copying frames
        int32_t m_maxCopiedMs = 0;  // Max. time displaying / copying  frames
        int32_t m_nrCopied = 0;     // Number of frames displayed / copied so far
    };
    IWRAM_DATA FrameStatistics m_videoStats;
    IWRAM_DATA FrameStatistics m_audioStats;
#endif

    IWRAM_FUNC auto AudioBufferRequest() -> void
    {
        if (m_playing)
        {
            // still playing back. stop both timers
            REG_TM0CNT_H &= ~TIMER_START;
            REG_TM1CNT_H &= ~TIMER_START;
            // stop DMA channels
            REG_DMA1CNT &= ~DMA_ENABLE;
            if (m_mediaInfo.audio.channels == 2)
            {
                REG_DMA2CNT &= ~DMA_ENABLE;
            }
            if (m_audioFramesDecoded > 0)
            {
                // swap sample buffers
                std::swap(m_audioPlayBuffer, m_audioBackBuffer);
                std::swap(m_audioPlayBufferChannelSize, m_audioBackBufferChannelSize);
                // calculate buffer start
                const auto sampleBuffer0 = reinterpret_cast<uint32_t>(m_audioPlayBuffer);
                // set DMA channels to read from new buffer and restart DMA
                REG_DMA1SAD = sampleBuffer0;
                REG_DMA1CNT |= DMA_ENABLE;
                if (m_mediaInfo.audio.channels == 2)
                {
                    // align output buffer to next word boundary
                    const auto sampleBuffer1 = (sampleBuffer0 + m_audioPlayBufferChannelSize + 3) & 0xFFFFFFFC;
                    REG_DMA2SAD = sampleBuffer1;
                    REG_DMA2CNT |= DMA_ENABLE;
                }
                // set timer 1 count to number of words in new buffer
                REG_TM1CNT_L = 65536U - m_audioPlayBufferChannelSize;
                // start both timers
                REG_TM0CNT_H |= TIMER_START;
                REG_TM1CNT_H |= TIMER_START;
                // we've used up the decoded frame
                m_audioFramesDecoded = 0;
            }
            // request more audio frames for playback
            ++m_audioFramesRequested;
        }
        else
        {
            // this was the last frame. turn off sounds
            REG_SOUNDCNT_X = 0;
            // stop both timers
            REG_TM0CNT_H = 0;
            REG_TM1CNT_H = 0;
            // stop DMA channels
            REG_DMA1CNT = 0;
            if (m_mediaInfo.audio.channels == 2)
            {
                REG_DMA2CNT = 0;
            }
            m_audioFramesRequested = 0;
        }
    }

    IWRAM_FUNC auto VideoFrameRequest() -> void
    {
        if (m_playing)
        {
            // request more video frames for playback
            ++m_videoFramesRequested;
        }
        else
        {
            // this was the last frame. stop requesting frames
            m_videoFramesRequested = 0;
        }
    }

    auto Init(const uint32_t *mediaSrc, uint32_t *videoScratchPad, uint32_t videoScratchPadSize, uint32_t vramStride, uint32_t *audioScratchPad, uint32_t audioScratchPadSize) -> void
    {
        // read file header
        m_mediaInfo = IO::Vid2h::GetInfo(mediaSrc);
        if (m_mediaInfo.contentType & IO::FileType::Video)
        {
            // set up video buffers
            m_videoScratchPad = videoScratchPad;
            m_videoScratchPadSize = videoScratchPadSize;
            m_vramStride = vramStride;
        }
        if (m_mediaInfo.contentType & IO::FileType::Audio)
        {
            // set up audio buffers
            m_audioPlayBuffer = audioScratchPad;
            m_audioBackBuffer = audioScratchPad + audioScratchPadSize / (2 * 4);
            m_audioBufferSize = audioScratchPadSize / 2;
        }
    }

    auto SetClearColor(uint16_t color) -> void
    {
        m_clearColor = color;
    }

    auto GetInfo() -> const IO::Vid2h::Info &
    {
        return m_mediaInfo;
    }

    IWRAM_FUNC auto ReadAndQueueNextFrame() -> void
    {
        m_mediaFrame = IO::Vid2h::GetNextFrame(m_mediaInfo, m_mediaFrame);
        if (m_mediaFrame.dataType == IO::FrameType::Pixels)
        {
            // queue video frame
            m_queuedVideoFrame = m_mediaFrame;
        }
        else if (m_mediaFrame.dataType == IO::FrameType::Colormap)
        {
            // queue color map frame
            m_queuedColorMapFrame = m_mediaFrame;
        }
        else if (m_mediaFrame.dataType == IO::FrameType::Audio)
        {
            // queue audio frame
            m_queuedAudioFrame = m_mediaFrame;
        }
    }

    IWRAM_FUNC auto DecodeAudioFrame() -> void
    {
#ifdef DEBUG_PLAYER
        auto startTime = Time::now();
#endif
        auto [audioDecodedFrame, audioDecodedFrameSize] = DecodeAudio(m_audioBackBuffer, m_audioBufferSize, m_mediaInfo, m_queuedAudioFrame);
        m_audioBackBufferChannelSize = audioDecodedFrameSize / m_mediaInfo.audio.channels;
        m_queuedAudioFrame.data = nullptr;
        ++m_audioFramesDecoded;
#ifdef DEBUG_PLAYER
        auto duration = Time::now() * 1000 - startTime * 1000;
        m_audioStats.m_accDecodedMs += duration;
        m_audioStats.m_maxDecodedMs = m_audioStats.m_maxDecodedMs < duration ? duration : m_audioStats.m_maxDecodedMs;
        m_audioStats.m_nrDecoded++;
#endif
    }

    IWRAM_FUNC auto DecodeVideoFrame() -> void
    {
#ifdef DEBUG_PLAYER
        auto startTime = Time::now();
#endif
        auto [videoDecodedFrame, videoDecodedFramesize] = DecodeVideo(m_videoScratchPad, m_videoScratchPadSize, m_mediaInfo, m_queuedVideoFrame);
        m_videoDecodedFrame = videoDecodedFrame;
        m_videoDecodedFrameSize = videoDecodedFramesize;
        m_queuedVideoFrame.data = nullptr;
        ++m_videoFramesDecoded;
#ifdef DEBUG_PLAYER
        auto duration = Time::now() * 1000 - startTime * 1000;
        m_videoStats.m_accDecodedMs += duration;
        m_videoStats.m_maxDecodedMs = m_videoStats.m_maxDecodedMs < duration ? duration : m_videoStats.m_maxDecodedMs;
        m_videoStats.m_nrDecoded++;
#endif
    }

    IWRAM_FUNC auto BlitVideoFrame() -> void
    {
#ifdef DEBUG_PLAYER
        auto startTime = Time::now();
#endif
        const uint32_t videoLineStride8 = ((m_mediaInfo.video.bitsPerPixel + 7) / 8) * m_mediaInfo.video.width;
        if (videoLineStride8 == m_vramStride)
        {
            // video fiulls screen completely. simply copy the data over
            Memory::memcpy32((void *)VRAM, m_videoDecodedFrame, m_videoDecodedFrameSize / 4);
        }
        else
        {
            // video does not fill screen. copy line by line
            const uint32_t videoLineStride32 = videoLineStride8 / 4;
            auto videoPtr32 = m_videoDecodedFrame;
            auto vramPtr8 = (uint8_t *)VRAM;
            for (uint32_t y = 0; y < m_mediaInfo.video.height; ++y)
            {
                Memory::memcpy32(vramPtr8, videoPtr32, videoLineStride32);
                videoPtr32 += videoLineStride32;
                vramPtr8 += m_vramStride;
            }
        }
#ifdef DEBUG_PLAYER
        auto duration = Time::now() * 1000 - startTime * 1000;
        m_videoStats.m_accCopiedMs += duration;
        m_videoStats.m_maxCopiedMs = m_videoStats.m_maxCopiedMs < duration ? duration : m_videoStats.m_maxCopiedMs;
        m_videoStats.m_nrCopied++;
#endif
    }

    auto Play() -> void
    {
        if (!m_playing)
        {
            m_mediaFrame.data = nullptr;
            m_queuedAudioFrame.data = nullptr;
            m_queuedVideoFrame.data = nullptr;
            m_queuedColorMapFrame.data = nullptr;
            m_audioFramesRequested = 0;
            m_audioFramesDecoded = 0;
            m_videoFramesRequested = 0;
            m_videoFramesDecoded = 0;
            m_playing = true;
#ifdef DEBUG_PLAYER
            m_videoStats = FrameStatistics{};
            m_audioStats = FrameStatistics{};
            Time::start();
#endif
            // load and decode initial frame(s)
            if (m_mediaInfo.contentType & IO::FileType::Audio)
            {
                // disable all sounds for now
                REG_SOUNDCNT_X = 0;
                // fill audio buffers with silence
                Memory::memset32(m_audioBackBuffer, 0, m_audioBufferSize / 4);
                // enable DMA 1 to copy words to sound FIFO A
                REG_DMA1DAD = reinterpret_cast<uint32_t>(&REG_FIFO_A);                        // write to sound FIFO A
                REG_DMA1CNT = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT | DMA32 | DMA_SPECIAL; // start DMA later
                if (m_mediaInfo.audio.channels == 1)
                {
                    // set DMA channel A to output to left and right at 100% and reset FIFO. by default timer 0 is used for both channels
                    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
                }
                else if (m_mediaInfo.audio.channels == 2)
                {
                    // enable DMA 2 to copy words to sound FIFO B
                    REG_DMA2DAD = reinterpret_cast<uint32_t>(&REG_FIFO_B);                        // write to sound FIFO B
                    REG_DMA2CNT = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT | DMA32 | DMA_SPECIAL; // start DMA later
                    // set DMA channel A to output to left at 100% and reset FIFO
                    // set DMA channel B to output to right at 100% and reset FIFO
                    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_RESET_FIFO | SNDB_VOL_100 | SNDB_R_ENABLE | SNDB_RESET_FIFO;
                }
                // set up timer 0 to increase with sample rate = 1 / sample rate (where 16777216 == 1s) and set divider to 0 == 1/1
                REG_TM0CNT_L = 65536U - (uint64_t(16777216) / m_mediaInfo.audio.sampleRateHz);
                REG_TM0CNT_H = 0; // start timer later
                // set timer 1 to cascade from timer 0, issue IRQ to swap sample buffers and set divider to 0 == 1/1
                irqSet(irqMASKS::IRQ_TIMER1, AudioBufferRequest);
                irqEnable(irqMASKS::IRQ_TIMER1);
                REG_TM1CNT_L = 0;                           // set up number of samples in buffer later
                REG_TM1CNT_H = TIMER_COUNT | TIMER_IRQ | 0; // start timer later
                // read the first video frame from media data
                while (m_queuedAudioFrame.data == nullptr)
                {
                    ReadAndQueueNextFrame();
                }
                // decode the first audio frame
                DecodeAudioFrame();
            }
            if (m_mediaInfo.contentType & IO::FileType::Video)
            {
                // fill background and scratch pad to clear color
                const uint32_t clearColor = static_cast<uint32_t>(m_clearColor) << 16 | m_clearColor;
                Memory::memset32((void *)VRAM, clearColor, m_mediaInfo.video.width * m_mediaInfo.video.height / 2);
                Memory::memset32(m_videoScratchPad, clearColor, m_videoScratchPadSize / 4);
                // set up timer 2 to increase with video frame interval = 1 / fps (where 65536 == 1s). frames/s are in 16:16 format
                // set timer 2 divider to 2 == 1/256 -> 16*1024*1024 cycles/s / 256 = 65536/s
                irqSet(irqMASKS::IRQ_TIMER2, VideoFrameRequest);
                irqEnable(irqMASKS::IRQ_TIMER2);
                REG_TM2CNT_L = 65536U - ((uint64_t(65536U) << 16) / m_mediaInfo.video.frameRateHz);
                REG_TM2CNT_H = TIMER_IRQ | 2; // start timer later
                // read the first video frame from media data
                while (m_queuedVideoFrame.data == nullptr)
                {
                    ReadAndQueueNextFrame();
                }
                // decode the first video frame
                DecodeVideoFrame();
            }
            // now display / play initial data
            if (m_mediaInfo.contentType & IO::FileType::Video)
            {
                // blit first frame to screen
                BlitVideoFrame();
                // start timer for video frame rate
                REG_TM2CNT_H |= TIMER_START;
            }
            if (m_mediaInfo.contentType & IO::FileType::Audio)
            {
                // start and enable sound playback
                REG_SOUNDCNT_X = SOUND3_PLAY;
                AudioBufferRequest();
            }
        }
    }

    IWRAM_FUNC auto HasMoreFrames() -> bool
    {
        return m_playing && IO::Vid2h::HasMoreFrames(m_mediaInfo, m_mediaFrame);
    }

    IWRAM_FUNC auto DecodeAndPlay() -> void
    {
        if (m_playing)
        {
            // blit video frame if one is pending
            if (m_videoFramesRequested > 0 && m_videoFramesDecoded > 0)
            {
                // we're waiting for a frame and have one. blit it!
                BlitVideoFrame();
                // we've used up the decoded frame
                m_videoFramesDecoded = 0;
            }
            // read frames from media data
            if (m_audioFramesRequested > 0 && m_queuedAudioFrame.data == nullptr)
            {
                ReadAndQueueNextFrame();
            }
            if (m_videoFramesRequested > 0 && m_queuedVideoFrame.data == nullptr)
            {
                ReadAndQueueNextFrame();
            }
            // decode queued frames if we have any (prefer audio here, as it is more time-critical)
            if (m_audioFramesDecoded < 1 && m_queuedAudioFrame.data != nullptr)
            {
                --m_audioFramesRequested;
                DecodeAudioFrame();
            }
            if (m_videoFramesDecoded < 1 && m_queuedVideoFrame.data != nullptr)
            {
                --m_videoFramesRequested;
                DecodeVideoFrame();
            }
        }
    }

    auto Stop() -> void
    {
        if (m_playing)
        {
            m_playing = false;
            m_audioFramesRequested = 0;
            m_videoFramesRequested = 0;
            // disable sound for now
            REG_SOUNDCNT_X = 0;
            // disable audio timers
            REG_TM0CNT_H = 0;
            REG_TM1CNT_H = 0;
            irqDisable(irqMASKS::IRQ_TIMER1);
            // disable audio DMAs
            REG_DMA1CNT = 0;
            if (m_mediaInfo.audio.channels == 2)
            {
                REG_DMA2CNT = 0;
            }
            // disable video timer
            REG_TM2CNT_H = 0;
            irqDisable(irqMASKS::IRQ_TIMER2);
#ifdef DEBUG_PLAYER
            Debug::printf("Audio avg. decode: %f ms (max. %f ms)", m_audioStats.m_accDecodedMs / m_audioStats.m_nrDecoded, m_audioStats.m_maxDecodedMs);
            Debug::printf("Video avg. decode: %f ms (max. %f ms)", m_videoStats.m_accDecodedMs / m_videoStats.m_nrDecoded, m_videoStats.m_maxDecodedMs);
            Debug::printf("Video avg. blit: %f ms (max. %f ms)", m_videoStats.m_accCopiedMs / m_videoStats.m_nrCopied, m_videoStats.m_maxCopiedMs);
            Time::stop();
#endif
        }
    }
}

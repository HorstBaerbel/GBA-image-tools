#include "videoplayer.h"

#include "memory/memory.h"
#include "subtitles.h"
#include "sys/base.h"
#include "vid2hdecoder.h"
#include <memory/dma.h>
#include <sys/interrupts.h>
#include <sys/sound.h>
#include <sys/timers.h>

// #define DEBUG_PLAYER
#ifdef DEBUG_PLAYER
#include "print/output.h"
#include "time.h"
#endif

// BBB no entropy coder
// Video avg. decode: 16.19 ms (max. 24.41 ms) ASM
// Audio avg. decode: 5.74 ms (max. 5.86 ms) ASM

// BBB LZ4 compression
// Video avg. decode: 20.04 ms (max. 35.16 ms) ASM
// Audio avg. decode: 5.99 ms (max. 6.84 ms) ASM

// AF 96% LZ4 compression
// Video avg. decode: 19.01 ms (max. 31.25 ms) ASM
// Audio avg. decode: 5.74 ms (max. 5.86 ms) ASM

// AF 95% LZ4 compression
// Video avg. decode: 18.26 ms (max. 29.30 ms) ASM
// Audio avg. decode: 5.74 ms (max. 5.86 ms) ASM

// Video avg. blit: 10.26 ms (max. 10.74 ms)

namespace Media
{
    // general
    IWRAM_DATA bool m_playing = false;
    IWRAM_DATA int32_t m_playTime = 0;
    IWRAM_DATA IO::Vid2h::Info m_mediaInfo;
    IWRAM_DATA IO::Vid2h::Frame m_mediaFrame;

    // audio
    IWRAM_DATA int32_t m_audioFrameTime = 0;
    IWRAM_DATA IO::Vid2h::Frame m_queuedAudioFrame{};       // Audio frame waiting to be decoded
    IWRAM_DATA volatile int16_t m_audioFramesDecoded = 0;   // Number of audio frames decoded in m_audioBackSampleBuffer
    IWRAM_DATA volatile int16_t m_audioFramesRequested = 0; // Number of audio frames requested by AudioBufferRequest()
    IWRAM_DATA uint32_t *m_audioPlayBuffer = nullptr;       // Pointer to planar data for left / mono channel and right channel currently playing
    IWRAM_DATA uint32_t *m_audioBackBuffer = nullptr;       // Pointer to planar data for left / mono channel and right channel currently decoding
    IWRAM_DATA uint16_t m_audioBufferSize8 = 0;             // Size of one audio buffer
    IWRAM_DATA uint16_t m_audioPlayBufferChannelSize8 = 0;  // Size of data stored per channel == number of samples per channel of buffer currently playing
    IWRAM_DATA uint16_t m_audioBackBufferChannelSize8 = 0;  // Size of data stored per channel == number of samples per channel of buffer currently decoding

    // video
    IWRAM_DATA uint32_t *m_videoScratchPad = nullptr;
    IWRAM_DATA uint32_t m_videoScratchPadSize8 = 0;
    IWRAM_DATA uint16_t m_vramLineStride8 = 0;
    IWRAM_DATA uint16_t m_vramPixelStride8 = 0;
    IWRAM_DATA uint16_t m_vramClearColor = 0;
    IWRAM_DATA uint32_t m_vramOffset8 = 0;
    IWRAM_DATA uint16_t m_videoPositionX = 0;
    IWRAM_DATA uint16_t m_videoPositionY = 0;
    IWRAM_DATA int32_t m_videoFrameTime = 0;
    IWRAM_DATA IO::Vid2h::Frame m_queuedColorMapFrame{};      // Color map frame waiting to be decoded
    IWRAM_DATA IO::Vid2h::Frame m_queuedVideoFrame{};         // Video frame waiting to be decoded
    IWRAM_DATA volatile int16_t m_videoFramesDecoded = 0;     // Number of video frames decoded in m_videoDecodedFrame
    IWRAM_DATA volatile int16_t m_videoFramesRequested = 0;   // Number of video frames requested by VideoFrameRequest()
    IWRAM_DATA const uint32_t *m_videoDecodedFrame = nullptr; // Pointer to decoded video frame
    IWRAM_DATA uint32_t m_videoDecodedFrameSize8 = 0;         // Size of decoded video frame

    // subtitle
    IWRAM_DATA bool m_subtitlesEnabled = true;
    IWRAM_DATA IO::Vid2h::Frame m_queuedSubtitleFrame{}; // Subtitle frame waiting to be decoded
    IWRAM_DATA Subtitles::Frame m_subtitleCurrent;       // Currently used/displayed subtitle
    IWRAM_DATA Subtitles::Frame m_subtitleNext;          // Decoded/next subtitle

#ifdef DEBUG_PLAYER
    struct FrameStatistics
    {
        int64_t m_accDecodedMs = 0; // Accumulated time decoding frames
        int64_t m_maxDecodedMs = 0; // Max. time decoding frames
        int64_t m_nrDecoded = 0;    // Number of frames decoded so far
        int64_t m_accCopiedMs = 0;  // Accumulated time displaying / copying frames
        int64_t m_maxCopiedMs = 0;  // Max. time displaying / copying  frames
        int64_t m_nrCopied = 0;     // Number of frames displayed / copied so far
    };
    IWRAM_DATA FrameStatistics m_videoStats;
    IWRAM_DATA FrameStatistics m_audioStats;
    IWRAM_DATA FrameStatistics m_subtitlesStats;
#endif

    IWRAM_FUNC auto AudioBufferRequest() -> void
    {
        m_playTime += m_audioFrameTime;
        if (m_playing)
        {
            // still playing back. stop both timers
            REG_TM0CNT_H &= ~TIMER_START;
            REG_TM1CNT_H &= ~TIMER_START;
            // stop DMA channels
            REG_DMA[1].control &= ~DMA_ENABLE;
            if (m_mediaInfo.audio.channels == 2)
            {
                REG_DMA[2].control &= ~DMA_ENABLE;
            }
            if (m_audioFramesDecoded > 0)
            {
                // swap sample buffers
                std::swap(m_audioPlayBuffer, m_audioBackBuffer);
                std::swap(m_audioPlayBufferChannelSize8, m_audioBackBufferChannelSize8);
                // calculate buffer start
                const auto sampleBuffer0 = reinterpret_cast<uint32_t>(m_audioPlayBuffer);
                // set DMA channels to read from new buffer and restart DMA
                REG_DMA[1].source = (uint32_t)sampleBuffer0;
                REG_DMA[1].control |= DMA_ENABLE;
                if (m_mediaInfo.audio.channels == 2)
                {
                    // align output buffer to next word boundary
                    const auto sampleBuffer1 = (sampleBuffer0 + m_audioPlayBufferChannelSize8 + 3) & 0xFFFFFFFC;
                    REG_DMA[2].source = (uint32_t)sampleBuffer1;
                    REG_DMA[2].control |= DMA_ENABLE;
                }
                // set timer 1 count to number of words in new buffer
                REG_TM1CNT_L = 65536U - m_audioPlayBufferChannelSize8;
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
            REG_DMA[1].control = 0;
            if (m_mediaInfo.audio.channels == 2)
            {
                REG_DMA[2].control = 0;
            }
            m_audioFramesRequested = 0;
        }
    }

    IWRAM_FUNC auto UpdateSubtitles() -> void
    {
#ifdef DEBUG_PLAYER
        auto startTime = Time::now();
#endif
        if (m_subtitleCurrent.text)
        {
            bool mustUpdateDisplay = false;
            if (m_playTime >= m_subtitleCurrent.endTimeS)
            {
                // remove subtitle
                Subtitles::clear();
                // make the next subtitle the current
                m_subtitleCurrent = m_subtitleNext;
                m_subtitleNext = {0, 0, nullptr};
                mustUpdateDisplay = true;
            }
            if (m_playTime < m_subtitleCurrent.endTimeS && m_playTime >= m_subtitleCurrent.startTimeS)
            {
                // count line breaks in subtitle
                const uint32_t nrOfLines = Subtitles::getNrOfLines(m_subtitleCurrent.text);
                auto string = m_subtitleCurrent.text;
                auto end = string;
                auto y = m_videoPositionY + m_mediaInfo.video.height - nrOfLines * (Subtitles::FontHeight + Subtitles::FontHeight / 2);
                for (uint32_t i = 0; i < nrOfLines; ++i)
                {
                    // find end of string
                    while (*end != '\0' && *end != '\n')
                    {
                        ++end;
                    }
                    // calculate subtitle length
                    const auto textLength = Subtitles::getScreenWidth(string, end);
                    // show subtitle centered at bottom of video
                    const auto x = m_videoPositionX + (m_mediaInfo.video.width - textLength) / 2;
                    Subtitles::printString(string, end, x, y);
                    string = end + 1;
                    end = string;
                    y += Subtitles::FontHeight + Subtitles::FontHeight / 2;
                }
                // clear start time so we don't display again
                m_subtitleCurrent.startTimeS = m_subtitleCurrent.endTimeS;
                mustUpdateDisplay = true;
            }
            if (mustUpdateDisplay)
            {
                Subtitles::display();
            }
#ifdef DEBUG_PLAYER
            auto duration = Time::now() * 1000 - now * 1000;
            m_subtitlesStats.m_accCopiedMs += duration;
            m_subtitlesStats.m_maxCopiedMs = m_subtitlesStats.m_maxCopiedMs < duration ? duration : m_subtitlesStats.m_maxCopiedMs;
            m_subtitlesStats.m_nrCopied++;
#endif
        }
    }

    IWRAM_FUNC auto VideoFrameRequest() -> void
    {
        m_playTime += m_videoFrameTime;
        if (m_playing)
        {
            // request more video frames for playback
            ++m_videoFramesRequested;
            // update subtitles
            if (m_mediaInfo.contentType & IO::FileType::Subtitles && m_subtitlesEnabled)
            {
                UpdateSubtitles();
            }
        }
        else
        {
            // this was the last frame. stop requesting frames
            m_videoFramesRequested = 0;
        }
    }

    auto Init(const uint32_t *media, const uint32_t mediaSize, uint32_t *videoScratchPad, const uint32_t videoScratchPadSize, const uint32_t vramLineStride8, const uint32_t vramPixelStride8, uint32_t *audioScratchPad, const uint32_t audioScratchPadSize) -> void
    {
        // read file header
        m_mediaInfo = IO::Vid2h::GetInfo(media, mediaSize);
        if (m_mediaInfo.contentType & IO::FileType::Video)
        {
            // set up video buffers
            m_videoScratchPad = videoScratchPad;
            m_videoScratchPadSize8 = videoScratchPadSize;
            m_vramLineStride8 = vramLineStride8;
            m_vramPixelStride8 = vramPixelStride8;
        }
        if (m_mediaInfo.contentType & IO::FileType::Audio)
        {
            // set up audio buffers
            m_audioPlayBuffer = audioScratchPad;
            m_audioBackBuffer = audioScratchPad + audioScratchPadSize / (2 * 4);
            m_audioBufferSize8 = audioScratchPadSize / 2;
        }
    }

    auto SetClearColor(uint16_t color) -> void
    {
        m_vramClearColor = color;
    }

    auto SetPosition(uint16_t x, uint16_t y) -> void
    {
        m_videoPositionX = x;
        m_videoPositionY = y;
        m_vramOffset8 = y * m_vramLineStride8 + x * m_vramPixelStride8;
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
            m_queuedVideoFrame = m_mediaFrame;
        }
        else if (m_mediaFrame.dataType == IO::FrameType::Colormap)
        {
            m_queuedColorMapFrame = m_mediaFrame;
        }
        else if (m_mediaFrame.dataType == IO::FrameType::Audio)
        {
            m_queuedAudioFrame = m_mediaFrame;
        }
        else if (m_mediaFrame.dataType == IO::FrameType::Subtitles)
        {
            m_queuedSubtitleFrame = m_mediaFrame;
        }
    }

    IWRAM_FUNC auto DecodeAudioFrame() -> void
    {
#ifdef DEBUG_PLAYER
        auto startTime = Time::now();
#endif
        auto [audioDecodedFrame, audioDecodedFrameSize] = DecodeAudio(m_audioBackBuffer, m_audioBufferSize8, m_mediaInfo, m_queuedAudioFrame);
        m_audioBackBufferChannelSize8 = audioDecodedFrameSize / m_mediaInfo.audio.channels;
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
        auto vramPtr8 = (uint8_t *)VRAM + m_vramOffset8;
        auto [videoDecodedFrame, videoDecodedFramesize] = DecodeVideo(m_videoScratchPad, m_videoScratchPadSize8, vramPtr8, m_vramLineStride8, m_mediaInfo, m_queuedVideoFrame);
        m_videoDecodedFrame = videoDecodedFrame;
        m_videoDecodedFrameSize8 = videoDecodedFramesize;
        m_queuedVideoFrame.data = nullptr;
        ++m_videoFramesDecoded;
#ifdef DEBUG_PLAYER
        auto duration = Time::now() * 1000 - startTime * 1000;
        m_videoStats.m_accDecodedMs += duration;
        m_videoStats.m_maxDecodedMs = m_videoStats.m_maxDecodedMs < duration ? duration : m_videoStats.m_maxDecodedMs;
        m_videoStats.m_nrDecoded++;
#endif
    }

    IWRAM_FUNC auto DecodeSubtitlesFrame() -> void
    {
#ifdef DEBUG_PLAYER
        auto startTime = Time::now();
#endif
        const auto subtitle = DecodeSubtitles(m_queuedSubtitleFrame);
        m_queuedSubtitleFrame.data = nullptr;
        if (m_subtitleCurrent.text == nullptr)
        {
            m_subtitleCurrent.startTimeS = std::get<0>(subtitle);
            m_subtitleCurrent.endTimeS = std::get<1>(subtitle);
            m_subtitleCurrent.text = std::get<2>(subtitle);
        }
        else
        {
            m_subtitleNext.startTimeS = std::get<0>(subtitle);
            m_subtitleNext.endTimeS = std::get<1>(subtitle);
            m_subtitleNext.text = std::get<2>(subtitle);
        }
#ifdef DEBUG_PLAYER
        auto duration = Time::now() * 1000 - startTime * 1000;
        m_subtitlesStats.m_accDecodedMs += duration;
        m_subtitlesStats.m_maxDecodedMs = m_subtitlesStats.m_maxDecodedMs < duration ? duration : m_subtitlesStats.m_maxDecodedMs;
        m_subtitlesStats.m_nrDecoded++;
#endif
    }

    IWRAM_FUNC auto BlitVideoFrame() -> void
    {
#ifdef DEBUG_PLAYER
        auto startTime = Time::now();
#endif
        // copy video data to VRAM
        const uint32_t videoLineStride8 = ((m_mediaInfo.video.bitsPerPixel + 7) / 8) * m_mediaInfo.video.width;
        if (videoLineStride8 == m_vramLineStride8)
        {
            // video fills screen completely. simply copy the data over
            auto vramPtr8 = (uint8_t *)VRAM + m_vramOffset8;
            Memory::memcpy32(vramPtr8, m_videoDecodedFrame, m_videoDecodedFrameSize8 / 4);
        }
        else
        {
            // video does not fill screen. copy line by line
            const uint32_t videoLineStride32 = videoLineStride8 / 4;
            auto videoPtr32 = m_videoDecodedFrame;
            auto vramPtr8 = (uint8_t *)VRAM + m_vramOffset8;
            for (uint32_t y = 0; y < m_mediaInfo.video.height; ++y)
            {
                Memory::memcpy32(vramPtr8, videoPtr32, videoLineStride32);
                videoPtr32 += videoLineStride32;
                vramPtr8 += m_vramLineStride8;
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
            if (m_mediaInfo.contentType & IO::FileType::Audio)
            {
                m_queuedAudioFrame.data = nullptr;
                m_audioFramesRequested = 0;
                m_audioFramesDecoded = 0;
            }
            if (m_mediaInfo.contentType & IO::FileType::Video)
            {
                m_queuedVideoFrame.data = nullptr;
                m_queuedColorMapFrame.data = nullptr;
                m_videoFramesRequested = 0;
                m_videoFramesDecoded = 0;
            }
            if (m_mediaInfo.contentType & IO::FileType::Subtitles)
            {
                Subtitles::setup();
                m_queuedSubtitleFrame.data = nullptr;
                m_subtitleCurrent = {0, 0, nullptr};
                m_subtitleNext = {0, 0, nullptr};
            }
            m_playing = true;
            m_playTime = 0;
#ifdef DEBUG_PLAYER
            m_videoStats = FrameStatistics{};
            m_audioStats = FrameStatistics{};
            m_subtitlesStats = FrameStatistics{};
            Time::start();
#endif
            // load and decode initial frame(s)
            if (m_mediaInfo.contentType & IO::FileType::Audio)
            {
                // disable all sounds for now
                REG_SOUNDCNT_X = 0;
                // fill audio buffers with silence
                Memory::memset32(m_audioBackBuffer, 0, m_audioBufferSize8 / 4);
                // enable DMA 1 to copy words to sound FIFO A
                REG_DMA[1].destination = (uint32_t)&REG_FIFO_A;                                      // write to sound FIFO A
                REG_DMA[1].control = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT | DMA32 | DMA_SPECIAL; // start DMA later
                if (m_mediaInfo.audio.channels == 1)
                {
                    // set DMA channel A to output to left and right at 100% and reset FIFO. by default timer 0 is used for both channels
                    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
                }
                else if (m_mediaInfo.audio.channels == 2)
                {
                    // enable DMA 2 to copy words to sound FIFO B
                    REG_DMA[2].destination = (uint32_t)&REG_FIFO_B;                                      // write to sound FIFO B
                    REG_DMA[2].control = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT | DMA32 | DMA_SPECIAL; // start DMA later
                    // set DMA channel A to output to left at 100% and reset FIFO
                    // set DMA channel B to output to right at 100% and reset FIFO
                    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_RESET_FIFO | SNDB_VOL_100 | SNDB_R_ENABLE | SNDB_RESET_FIFO;
                }
                // set up timer 0 to increase with sample rate = 1 / sample rate (where 16777216 == 1s) and set divider to 0 == 1/1
                const uint32_t frameTime = uint64_t(16777216) / m_mediaInfo.audio.sampleRateHz;
                REG_TM0CNT_L = 65536U - frameTime;
                REG_TM0CNT_H = 0; // start timer later
                // set timer 1 to cascade from timer 0, issue IRQ to swap sample buffers and set divider to 0 == 1/1
                irqSet(IRQMask::IRQ_TIMER1, AudioBufferRequest);
                irqEnable(IRQMask::IRQ_TIMER1);
                REG_TM1CNT_L = 0;                           // set up number of samples in buffer later
                REG_TM1CNT_H = TIMER_COUNT | TIMER_IRQ | 0; // start timer later
                // read the first video frame from media data
                while (m_queuedAudioFrame.data == nullptr)
                {
                    ReadAndQueueNextFrame();
                }
                // decode the first audio frame
                DecodeAudioFrame();
                m_audioFrameTime = m_mediaInfo.contentType & IO::FileType::Video ? 0 : frameTime;
            }
            if (m_mediaInfo.contentType & IO::FileType::Video)
            {
                // fill background and scratch pad to clear color
                const uint32_t clearColor = static_cast<uint32_t>(m_vramClearColor) << 16 | m_vramClearColor;
                Memory::memset32((void *)VRAM, clearColor, m_mediaInfo.video.width * m_mediaInfo.video.height / 2);
                Memory::memset32(m_videoScratchPad, clearColor, m_videoScratchPadSize8 / 4);
                // set up timer 2 to increase with video frame interval = 1 / fps (where 65536 == 1s). frames/s are in 16:16 format
                // set timer 2 divider to 2 == 1/256 -> 16*1024*1024 cycles/s / 256 = 65536/s
                irqSet(IRQMask::IRQ_TIMER2, VideoFrameRequest);
                irqEnable(IRQMask::IRQ_TIMER2);
                const uint32_t frameTime = uint64_t(4294967296) / m_mediaInfo.video.frameRateHz;
                m_videoFrameTime = frameTime;
                REG_TM2CNT_L = 65536U - frameTime;
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
            if (m_mediaInfo.contentType & IO::FileType::Subtitles && m_subtitlesEnabled)
            {
                if (m_queuedSubtitleFrame.data != nullptr)
                {
                    DecodeSubtitlesFrame();
                    UpdateSubtitles();
                }
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
            if (m_queuedSubtitleFrame.data != nullptr)
            {
                DecodeSubtitlesFrame();
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
            irqDisable(IRQMask::IRQ_TIMER1);
            // disable audio DMAs
            REG_DMA[1].control = 0;
            if (m_mediaInfo.audio.channels == 2)
            {
                REG_DMA[2].control = 0;
            }
            // disable video timer
            REG_TM2CNT_H = 0;
            irqDisable(IRQMask::IRQ_TIMER2);
            // clean up subtitles
            Subtitles::cleanup();
            m_subtitleCurrent = {0, 0, nullptr};
            m_subtitleNext = {0, 0, nullptr};
#ifdef DEBUG_PLAYER
            Debug::printf("Audio avg. decode: %f ms (max. %f ms)", int32_t(m_audioStats.m_accDecodedMs / m_audioStats.m_nrDecoded), m_audioStats.m_maxDecodedMs);
            Debug::printf("Video avg. decode: %f ms (max. %f ms)", int32_t(m_videoStats.m_accDecodedMs / m_videoStats.m_nrDecoded), m_videoStats.m_maxDecodedMs);
            Debug::printf("Video avg. blit: %f ms (max. %f ms)", int32_t(m_videoStats.m_accCopiedMs / m_videoStats.m_nrCopied), m_videoStats.m_maxCopiedMs);
            Debug::printf("Subtitles avg. update: %f ms (max. %f ms)", int32_t(m_subtitlesStats.m_accCopiedMs / m_subtitlesStats.m_nrCopied), m_subtitlesStats.m_maxCopiedMs);
            Time::stop();
#endif
        }
    }
}

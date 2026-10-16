#include "mediaplayer.h"

#include "mediahelp.h"
#include "memory/memory.h"
#include "subtitles.h"
#include "sys/base.h"
#include "vid2hdecoder.h"
#include <memory/dma.h>
#include <sys/interrupts.h>
#include <sys/sound.h>
#include <sys/timers.h>

// #define DEBUG_VIDEOPLAYER
#ifdef DEBUG_VIDEOPLAYER
#include "print/output.h"
#include "time.h"
#endif

#define AUDIO_SAMPLERATE 32768 // m_mediaInfo.audio.sampleRateHz // define to 32768 to upsample audio to 32kHz

namespace MediaPlayer
{
    // sprites
    EWRAM_DATA uint16_t m_tileStartIndex = 0;
    EWRAM_DATA uint8_t m_spriteStartIndex = 0;
    EWRAM_DATA uint8_t m_paletteStartIndex = 0;

    // general
    IWRAM_DATA Features m_features = Features::None;
    IWRAM_DATA PlaybackType m_playbackType = PlaybackType::AudioVideo;
    IWRAM_DATA PlayState m_playState = PlayState::Stopped;
    IWRAM_DATA int32_t m_playTime = 0;
    IWRAM_DATA IO::Vid2h::Info m_mediaInfo{};
    IWRAM_DATA IO::Vid2h::Frame m_mediaFrame{};

    // audio
    IWRAM_DATA int32_t m_audioSampleTime = 0;                        // Time per audio sample in s in 0.32 format
    IWRAM_DATA IO::Vid2h::Frame m_queuedAudioFrame{};                // Audio frame waiting to be decoded
    IWRAM_DATA volatile int16_t m_audioFramesDecoded = 0;            // Number of audio frames decoded in m_audioBackSampleBuffer
    IWRAM_DATA volatile int16_t m_audioFramesRequested = 0;          // Number of audio frames requested by AudioBufferRequest()
    IWRAM_DATA uint32_t *m_audioScratchPad = nullptr;                // Audio scratch pad for decoding in EWRAM
    IWRAM_DATA uint8_t *m_audioSampleBuffer[2] = {nullptr, nullptr}; // Planar sample data for left and right channel in IWRAM
    IWRAM_DATA uint8_t *m_audioPlayPtr[2] = {nullptr, nullptr};      // Pointers to planar sample data for left and right channel currently playing in IWRAM
    IWRAM_DATA uint8_t *m_audioBackPtr[2] = {nullptr, nullptr};      // Pointers to planar sample data for left and right channel currently fro next frame in IWRAM
    IWRAM_DATA uint16_t m_audioBufferTrailingBytes8 = 0;             // Size of bytes trailing after current play buffer not being played this frame to make its size a multiple of 16 bytes
    IWRAM_DATA uint16_t m_audioPlayBufferChannelSize8 = 0;           // Size of data stored per channel == number of samples per channel of buffer currently playing
    IWRAM_DATA uint16_t m_audioBackBufferChannelSize8 = 0;           // Size of data stored per channel == number of samples per channel of buffer currently decoding

    // video
    IWRAM_DATA uint32_t *m_videoScratchPad = nullptr;
    IWRAM_DATA uint32_t m_videoScratchPadSize8 = 0;
    IWRAM_DATA uint16_t m_vramLineStride8 = 0;
    IWRAM_DATA uint16_t m_vramPixelStride8 = 0;
    IWRAM_DATA uint16_t m_vramClearColor = 0;
    IWRAM_DATA uint32_t m_vramOffset8 = 0;
    IWRAM_DATA uint16_t m_videoPositionX = 0;
    IWRAM_DATA uint16_t m_videoPositionY = 0;
    IWRAM_DATA int32_t m_videoFrameTime = 0;                  // Time per video frame in s in 16.16 format
    IWRAM_DATA IO::Vid2h::Frame m_queuedColorMapFrame{};      // Color map frame waiting to be decoded
    IWRAM_DATA IO::Vid2h::Frame m_queuedVideoFrame{};         // Video frame waiting to be decoded
    IWRAM_DATA volatile int16_t m_videoFramesDecoded = 0;     // Number of video frames decoded in m_videoDecodedFrame
    IWRAM_DATA volatile int16_t m_videoFramesRequested = 0;   // Number of video frames requested by VideoFrameRequest()
    IWRAM_DATA const uint32_t *m_videoDecodedFrame = nullptr; // Pointer to decoded video frame
    IWRAM_DATA uint32_t m_videoDecodedFrameSize8 = 0;         // Size of decoded video frame

    // subtitle
    IWRAM_DATA bool m_subtitlesEnabled = true;
    IWRAM_DATA IO::Vid2h::Frame m_queuedSubtitleFrame{}; // Subtitle frame waiting to be decoded
    IWRAM_DATA Subtitles::Frame m_subtitleCurrent{};     // Currently used/displayed subtitle
    IWRAM_DATA Subtitles::Frame m_subtitleNext{};        // Decoded/next subtitle

    // help
    IWRAM_DATA bool m_videoHelpEnabled = false;

#ifdef DEBUG_VIDEOPLAYER
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

    // ------ Some macros ----------------------------------------------------

#define HAS_FEATURE_HELP (m_features & Features::Help)
#define HAS_FEATURE_SUBTILES (m_features & Features::Subtitles)
#define HAS_PLAYBACK_AUDIO (m_playbackType & PlaybackType::Audio)
#define HAS_PLAYBACK_VIDEO (m_playbackType & PlaybackType::Video)
#define HAS_MEDIA_AUDIO (m_mediaInfo.contentType & IO::FileType::Audio)
#define HAS_MEDIA_VIDEO (m_mediaInfo.contentType & IO::FileType::Video)
#define HAS_MEDIA_SUBTITLES (m_features & Features::Subtitles)
#define AUDIO_PLAYBACK_ON (HAS_MEDIA_AUDIO && HAS_PLAYBACK_AUDIO)
#define VIDEO_PLAYBACK_ON (HAS_MEDIA_VIDEO && HAS_PLAYBACK_VIDEO)
#define SUBTITLES_PLAYBACK_ON (HAS_MEDIA_SUBTITLES && HAS_FEATURE_SUBTILES)

    // ------ Video help functions ----------------------------------------------------

    auto IsVideoHelpEnabled() -> bool
    {
        return m_videoHelpEnabled;
    }

    auto EnableVideoHelp(bool enable) -> void
    {
        // check if the feature is enabled
        if (HAS_FEATURE_HELP)
        {
            m_videoHelpEnabled = enable;
            MediaHelp::SetVisible(enable);
            MediaHelp::Present();
            if (SUBTITLES_PLAYBACK_ON && m_subtitlesEnabled)
            {
                Subtitles::SetVisible(!enable);
                Subtitles::Present();
            }
        }
    }

    // ------ Subtitle functions ----------------------------------------------------

    IWRAM_FUNC auto UpdateSubtitles() -> void
    {
        if (m_subtitleCurrent.text)
        {
#ifdef DEBUG_VIDEOPLAYER
            auto startTime = Time::now();
#endif
            bool mustUpdateDisplay = false;
            if (m_playTime >= m_subtitleCurrent.endTimeS)
            {
                // remove subtitle
                Subtitles::Clear();
                // make the next subtitle the current
                m_subtitleCurrent = m_subtitleNext;
                m_subtitleNext = {0, 0, nullptr};
                mustUpdateDisplay = true;
            }
            if (m_playTime < m_subtitleCurrent.endTimeS && m_playTime >= m_subtitleCurrent.startTimeS)
            {
                // count line breaks in subtitle
                const uint32_t nrOfLines = Subtitles::GetNrOfLines(m_subtitleCurrent.text);
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
                    const auto textLength = Subtitles::GetScreenWidth(string, end);
                    // show subtitle centered at bottom of video
                    const auto x = m_videoPositionX + (m_mediaInfo.video.width - textLength) / 2;
                    Subtitles::PrintString(string, end, x, y);
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
                Subtitles::Present();
            }
#ifdef DEBUG_VIDEOPLAYER
            auto duration = Time::now() * 1000 - startTime * 1000;
            m_subtitlesStats.m_accCopiedMs += duration;
            m_subtitlesStats.m_maxCopiedMs = m_subtitlesStats.m_maxCopiedMs < duration ? duration : m_subtitlesStats.m_maxCopiedMs;
            m_subtitlesStats.m_nrCopied++;
#endif
        }
    }

    auto AreSubtitlesEnabled() -> bool
    {
        return m_subtitlesEnabled;
    }

    auto EnableSubtitles(bool enable) -> void
    {
        // check if the subtitles are generally available
        if (SUBTITLES_PLAYBACK_ON)
        {
            // check if they are enable, but should be disabled
            if (m_subtitlesEnabled && !enable)
            {
                // clear all subtitles / sprites
                Subtitles::Clear();
                Subtitles::Present();
            }
            m_subtitlesEnabled = enable;
        }
    }

    // ------ Timer IRQ handlers ----------------------------------------------------

    IWRAM_FUNC auto AudioBufferRequest() -> void
    {
        if (m_playState == PlayState::Playing)
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
                std::swap(m_audioPlayPtr[0], m_audioBackPtr[0]);
                std::swap(m_audioPlayPtr[1], m_audioBackPtr[1]);
                std::swap(m_audioPlayBufferChannelSize8, m_audioBackBufferChannelSize8);
                // set DMA channels to read from new buffer and restart DMA
                REG_DMA[1].source = reinterpret_cast<uint32_t>(m_audioPlayPtr[0]);
                REG_DMA[1].control |= DMA_ENABLE;
                if (m_mediaInfo.audio.channels == 2)
                {
                    REG_DMA[2].source = reinterpret_cast<uint32_t>(m_audioPlayPtr[1]);
                    REG_DMA[2].control |= DMA_ENABLE;
                }
                // audio may have been started or resumed. check if we need to (re-)enable it
                if (REG_SOUNDCNT_X == 0)
                {
                    if (m_mediaInfo.audio.channels == 1)
                    {
                        // set DMA channel A to output to left and right at 100%. by default timer 0 is used for both channels
                        REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
                    }
                    else if (m_mediaInfo.audio.channels == 2)
                    {
                        // set DMA channel A to output to left at 100% and reset FIFO. by default timer 0 is used for both channels
                        // set DMA channel B to output to right at 100% and reset FIFO. by default timer 0 is used for both channels
                        REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_RESET_FIFO | SNDB_VOL_100 | SNDB_R_ENABLE | SNDB_RESET_FIFO;
                    }
                    REG_SOUNDBIAS = SND_BIAS_LEVEL_DEFAULT | SND_SAMPLE_FREQ_DEFAULT;
                    REG_SOUNDCNT_X = SOUND3_PLAY;
                }
                // set timer 1 count to number of words in new buffer
                REG_TM1CNT_L = 65536U - m_audioPlayBufferChannelSize8;
                // start both timers
                REG_TM0CNT_H |= TIMER_START;
                REG_TM1CNT_H |= TIMER_START;
                // we've used up the decoded frame
                m_audioFramesDecoded = 0;
            }
            else
            {
                // we expected decoded audio frames here, but we seem to be skipping frames
                // at least make sure AudioBufferRequest() gets called for next frame
                // set timer 1 count to number of words in new buffer (technically this is incorrect, but well...)
                REG_TM1CNT_L = 65536U - m_audioPlayBufferChannelSize8;
                // start both timers
                REG_TM0CNT_H |= TIMER_START;
                REG_TM1CNT_H |= TIMER_START;
            }
            // update play time (use back buffer, as this is the one we've played before)
            m_playTime += (uint64_t(m_audioSampleTime) * m_audioBackBufferChannelSize8) >> 16;
            // request more audio frames for playback
            ++m_audioFramesRequested;
            // check if the subtitles are generally available
            if (SUBTITLES_PLAYBACK_ON)
            {
                // update subtitles here only if we're not playing video
                if (m_subtitlesEnabled && !VIDEO_PLAYBACK_ON)
                {
                    UpdateSubtitles();
                }
            }
        }
        else
        {
            // stop both timers
            REG_TM0CNT_H = 0;
            REG_TM1CNT_H = 0;
            // this was the last frame. turn off sounds
            REG_SOUNDCNT_X = 0;
            REG_SOUNDBIAS = 0;
            // stop DMA channels
            REG_DMA[1].control = 0;
            if (m_mediaInfo.audio.channels == 2)
            {
                REG_DMA[2].control = 0;
            }
            m_audioFramesRequested = 0;
        }
    }

    IWRAM_FUNC auto VideoFrameRequest() -> void
    {
        if (m_playState == PlayState::Playing)
        {
            m_playTime += m_videoFrameTime;
            // request more video frames for playback
            ++m_videoFramesRequested;
            // check if the subtitles are generally available
            if (SUBTITLES_PLAYBACK_ON && m_subtitlesEnabled)
            {
                // update subtitles
                UpdateSubtitles();
            }
        }
        else
        {
            // this was the last frame. stop requesting frames
            m_videoFramesRequested = 0;
        }
    }

    // ------ Get/Set ----------------------------------------------------

    auto SetFeatures(Features features) -> void
    {
        m_features = features;
    }

    auto SetDisplayInfo(const uint32_t vramLineStride8, const uint32_t vramPixelStride8) -> void
    {
        m_vramLineStride8 = vramLineStride8;
        m_vramPixelStride8 = vramPixelStride8;
    }

    auto SetClearColor(uint16_t color) -> void
    {
        m_vramClearColor = color;
    }

    auto SetSpriteInfo(uint32_t spriteStartIndex, uint32_t tileStartIndex, uint16_t paletteStartIndex) -> void
    {
        m_tileStartIndex = tileStartIndex;
        m_spriteStartIndex = spriteStartIndex;
        m_paletteStartIndex = paletteStartIndex;
    }

    auto SetPosition(uint16_t x, uint16_t y) -> void
    {
        m_videoPositionX = x;
        m_videoPositionY = y;
        m_vramOffset8 = y * m_vramLineStride8 + x * m_vramPixelStride8;
    }

    // ------ Frame handling ----------------------------------------------------

    IWRAM_FUNC auto ReadAndQueueNextFrame() -> void
    {
        m_mediaFrame = IO::Vid2h::GetNextFrame(m_mediaInfo, m_mediaFrame);
        if (HAS_PLAYBACK_VIDEO && m_mediaFrame.dataType == IO::FrameType::Pixels)
        {
            m_queuedVideoFrame = m_mediaFrame;
        }
        else if (HAS_PLAYBACK_VIDEO && m_mediaFrame.dataType == IO::FrameType::Colormap)
        {
            m_queuedColorMapFrame = m_mediaFrame;
        }
        else if (HAS_PLAYBACK_AUDIO && m_mediaFrame.dataType == IO::FrameType::Audio)
        {
            m_queuedAudioFrame = m_mediaFrame;
        }
        else if (HAS_FEATURE_SUBTILES && m_mediaFrame.dataType == IO::FrameType::Subtitles)
        {
            m_queuedSubtitleFrame = m_mediaFrame;
        }
    }

    IWRAM_FUNC auto DecodeAudioFrame() -> void
    {
#ifdef DEBUG_VIDEOPLAYER
        auto startTime = Time::now();
#endif
        // if we're playing the lower half of the buffer set back buffer to end of play buffer
        // if we're playing the upper half of the buffer set back buffer to start of sample buffer
        m_audioBackPtr[0] = m_audioPlayPtr[0] == m_audioSampleBuffer[0] ? m_audioSampleBuffer[0] + m_audioPlayBufferChannelSize8 : m_audioSampleBuffer[0];
        m_audioBackPtr[1] = m_audioPlayPtr[1] == m_audioSampleBuffer[1] ? m_audioSampleBuffer[1] + m_audioPlayBufferChannelSize8 : m_audioSampleBuffer[1];
        // move the decode pointers back m_audioBufferTrailingBytes8 to the actual end of the play buffers
        uint8_t *decodePtr[2] = {
            m_audioBackPtr[0] + m_audioBufferTrailingBytes8,
            m_audioBackPtr[1] + m_audioBufferTrailingBytes8};
        // now decode next audio frame
        const auto decodedBufferSize8 = Vid2hDecoder::DecodeAudio(decodePtr, m_audioScratchPad, m_mediaInfo, m_queuedAudioFrame);
        // we actually have m_audioBufferTrailingBytes8 more than decodedBufferSize8 per buffer
        const uint16_t combinedBufferSize8 = decodedBufferSize8 / m_mediaInfo.audio.channels + m_audioBufferTrailingBytes8;
        // make next buffer size a multiple of 16, round down. this might leave trailing bytes at the end of the next buffer
        m_audioBackBufferChannelSize8 = combinedBufferSize8 & 0xFFF0;
        m_audioBufferTrailingBytes8 = combinedBufferSize8 - m_audioBackBufferChannelSize8;
        // copy extra bytes from end of back buffers to start of play buffers (they will be the new back buffers after the swap)
        if (m_audioBackPtr[0] != m_audioSampleBuffer[0] && m_audioBufferTrailingBytes8 > 0)
        {
            for (uint32_t channel = 0; channel < m_mediaInfo.audio.channels; ++channel)
            {
                auto dstPtr = m_audioPlayPtr[channel];
                auto srcPtr = const_cast<const uint8_t *>(m_audioBackPtr[channel]) + m_audioBackBufferChannelSize8;
                for (uint32_t i = 0; i < m_audioBufferTrailingBytes8; ++i)
                {
                    *dstPtr++ = *srcPtr++;
                }
            }
        }
        // if this the last frame, add silence to the end of the back buffer if it was cut short
        if (m_queuedAudioFrame.index == (m_mediaInfo.audio.nrOfFrames - 1) && m_audioBufferTrailingBytes8 > 0)
        {
            for (uint32_t channel = 0; channel < m_mediaInfo.audio.channels; ++channel)
            {
                auto dstPtr = m_audioBackPtr[channel] + m_audioBackBufferChannelSize8 + m_audioBufferTrailingBytes8;
                for (uint32_t i = 0; i < (16U - m_audioBufferTrailingBytes8); ++i)
                {
                    *dstPtr++ = 0;
                }
            }
            m_audioBackBufferChannelSize8 += 16;
        }
        m_queuedAudioFrame.data = nullptr;
        ++m_audioFramesDecoded;
#ifdef DEBUG_VIDEOPLAYER
        auto duration = Time::now() * 1000 - startTime * 1000;
        m_audioStats.m_accDecodedMs += duration;
        m_audioStats.m_maxDecodedMs = m_audioStats.m_maxDecodedMs < duration ? duration : m_audioStats.m_maxDecodedMs;
        m_audioStats.m_nrDecoded++;
#endif
    }

    IWRAM_FUNC auto DecodeVideoFrame() -> void
    {
#ifdef DEBUG_VIDEOPLAYER
        auto startTime = Time::now();
#endif
        auto vramPtr8 = (uint8_t *)VRAM + m_vramOffset8;
        auto [videoDecodedFrame, videoDecodedFramesize] = Vid2hDecoder::DecodeVideo(m_videoScratchPad, m_videoScratchPadSize8, vramPtr8, m_vramLineStride8, m_mediaInfo, m_queuedVideoFrame);
        m_videoDecodedFrame = videoDecodedFrame;
        m_videoDecodedFrameSize8 = videoDecodedFramesize;
        m_queuedVideoFrame.data = nullptr;
        ++m_videoFramesDecoded;
#ifdef DEBUG_VIDEOPLAYER
        auto duration = Time::now() * 1000 - startTime * 1000;
        m_videoStats.m_accDecodedMs += duration;
        m_videoStats.m_maxDecodedMs = m_videoStats.m_maxDecodedMs < duration ? duration : m_videoStats.m_maxDecodedMs;
        m_videoStats.m_nrDecoded++;
#endif
    }

    IWRAM_FUNC auto DecodeSubtitlesFrame() -> void
    {
#ifdef DEBUG_VIDEOPLAYER
        auto startTime = Time::now();
#endif
        const auto subtitle = Vid2hDecoder::DecodeSubtitles(m_queuedSubtitleFrame);
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
#ifdef DEBUG_VIDEOPLAYER
        auto duration = Time::now() * 1000 - startTime * 1000;
        m_subtitlesStats.m_accDecodedMs += duration;
        m_subtitlesStats.m_maxDecodedMs = m_subtitlesStats.m_maxDecodedMs < duration ? duration : m_subtitlesStats.m_maxDecodedMs;
        m_subtitlesStats.m_nrDecoded++;
#endif
    }

    IWRAM_FUNC auto BlitVideoFrame() -> void
    {
#ifdef DEBUG_VIDEOPLAYER
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
#ifdef DEBUG_VIDEOPLAYER
        auto duration = Time::now() * 1000 - startTime * 1000;
        m_videoStats.m_accCopiedMs += duration;
        m_videoStats.m_maxCopiedMs = m_videoStats.m_maxCopiedMs < duration ? duration : m_videoStats.m_maxCopiedMs;
        m_videoStats.m_nrCopied++;
#endif
    }

    auto Play(const uint32_t *media, const uint32_t mediaSize, PlaybackType playbackType) -> void
    {
        if (m_playState == PlayState::Stopped)
        {
            m_playState = PlayState::Playing;
            m_playbackType = playbackType;
            m_playTime = 0;
            m_videoHelpEnabled = false;
            // read file header
            m_mediaInfo = IO::Vid2h::GetInfo(media, mediaSize);
            if (VIDEO_PLAYBACK_ON)
            {
                // allocate video back buffer and scratch pad
                const uint32_t videoBuffersize = m_mediaInfo.video.width * m_mediaInfo.video.height * ((m_mediaInfo.video.bitsPerPixel + 7) / 8);
                m_videoScratchPadSize8 = videoBuffersize + m_mediaInfo.video.memoryNeeded + 32;
                m_videoScratchPad = reinterpret_cast<uint32_t *>(Memory::malloc_EWRAM(m_videoScratchPadSize8));
                // fill background with clear color
                const uint32_t clearColor = static_cast<uint32_t>(m_vramClearColor) << 16 | m_vramClearColor;
                Memory::memset32((void *)VRAM, clearColor, m_mediaInfo.video.width * m_mediaInfo.video.height / 2);
                // clear video frame information
                m_queuedVideoFrame.data = nullptr;
                m_queuedColorMapFrame.data = nullptr;
                m_videoFramesRequested = 0;
                m_videoFramesDecoded = 0;
            }
            if (AUDIO_PLAYBACK_ON)
            {
                // allocate audio decoding scratch pad (for both channels)
                m_audioScratchPad = reinterpret_cast<uint32_t *>(Memory::malloc_EWRAM(m_mediaInfo.audio.memoryNeeded));
                // initialize audio decoder / resampler
                Vid2hDecoder::DecodeAudioInit(m_mediaInfo, AUDIO_SAMPLERATE);
                // we're resampling to 32kHz / 8 bit for GBA output, so calculate needed buffer size plus 16 bytes for safety
                const uint32_t singleAudioBufferSize = (Vid2hDecoder::GetMaxAudioBufferSize(m_mediaInfo, 8, AUDIO_SAMPLERATE) & 0xFFFFFFF0) + 16;
                // allocate audio buffers
                // we need singleAudioBufferSize for a single buffer of mono or stereo audio. we need twice that for playback for double buffering
                m_audioSampleBuffer[0] = reinterpret_cast<uint8_t *>(Memory::malloc_IWRAM(2 * singleAudioBufferSize));
                m_audioSampleBuffer[1] = m_audioSampleBuffer[0] + singleAudioBufferSize;
                m_audioPlayPtr[0] = m_audioSampleBuffer[0] + 16; // pretend we're playing the back half of the buffer for the initial frame decode
                m_audioPlayPtr[1] = m_audioSampleBuffer[1] + 16; // pretend we're playing the back half of the buffer for the initial frame decode
                m_audioBackPtr[0] = m_audioSampleBuffer[0];
                m_audioBackPtr[1] = m_audioSampleBuffer[1];
                m_audioBufferTrailingBytes8 = 0;
                m_audioPlayBufferChannelSize8 = 0;
                m_audioBackBufferChannelSize8 = 0;
                // fill audio buffers with silence
                Memory::memset32(m_audioSampleBuffer[0], 0, 2 * singleAudioBufferSize / 4);
                // clear audio frame information
                m_queuedAudioFrame.data = nullptr;
                m_audioFramesRequested = 0;
                m_audioFramesDecoded = 0;
            }
            // enable video help only if requested
            if (HAS_FEATURE_HELP)
            {
                MediaHelp::Setup(m_spriteStartIndex, m_tileStartIndex, m_paletteStartIndex);
                MediaHelp::SetSymbolEnabled(MediaHelp::Symbol::Subtitles, false);
                MediaHelp::SetSymbolEnabled(MediaHelp::Symbol::Previous, false);
                MediaHelp::SetSymbolEnabled(MediaHelp::Symbol::Next, false);
            }
            if (SUBTITLES_PLAYBACK_ON)
            {
                // set up subtitle sprites
                Subtitles::Setup(m_spriteStartIndex + MediaHelp::SpritesInUse(), m_tileStartIndex + MediaHelp::TilesInUse(), m_paletteStartIndex + 1);
                m_queuedSubtitleFrame.data = nullptr;
                m_subtitleCurrent = {0, 0, nullptr};
                m_subtitleNext = {0, 0, nullptr};
                MediaHelp::SetSymbolEnabled(MediaHelp::Symbol::Subtitles, true);
            }
            m_mediaFrame.data = nullptr;
#ifdef DEBUG_VIDEOPLAYER
            m_videoStats = FrameStatistics{};
            m_audioStats = FrameStatistics{};
            m_subtitlesStats = FrameStatistics{};
            Time::start();
#endif
            // load and decode initial frame(s)
            if (AUDIO_PLAYBACK_ON)
            {
                // disable all sounds for now
                REG_SOUNDCNT_X = 0;
                REG_SOUNDBIAS = 0;
                // set DMA 1 to copy words to sound FIFO A
                REG_DMA[1].destination = (uint32_t)&REG_FIFO_A;                                      // write to sound FIFO A
                REG_DMA[1].control = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT | DMA32 | DMA_SPECIAL; // start DMA later
                if (m_mediaInfo.audio.channels == 1)
                {
                    // set DMA channel A to output to left and right at 100% and reset FIFO. by default timer 0 is used for both channels
                    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
                    // prime sound FIFO with 16 bytes of silence (is this needed?)
                    // REG_FIFO_A = 0UL;
                    // REG_FIFO_A = 0UL;
                    // REG_FIFO_A = 0UL;
                    // REG_FIFO_A = 0UL;
                }
                else if (m_mediaInfo.audio.channels == 2)
                {
                    // set DMA 2 to copy words to sound FIFO B
                    REG_DMA[2].destination = (uint32_t)&REG_FIFO_B;                                      // write to sound FIFO B
                    REG_DMA[2].control = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT | DMA32 | DMA_SPECIAL; // start DMA later
                    // set DMA channel A to output to left at 100% and reset FIFO
                    // set DMA channel B to output to right at 100% and reset FIFO
                    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_RESET_FIFO | SNDB_VOL_100 | SNDB_R_ENABLE | SNDB_RESET_FIFO;
                    // prime sound FIFOs with 16 bytes of silence (is this needed?)
                    // REG_FIFO_A = 0UL;
                    // REG_FIFO_A = 0UL;
                    // REG_FIFO_A = 0UL;
                    // REG_FIFO_A = 0UL;
                    // REG_FIFO_B = 0UL;
                    // REG_FIFO_B = 0UL;
                    // REG_FIFO_B = 0UL;
                    // REG_FIFO_B = 0UL;
                }
                // set up timer 0 to increase with sample rate = 1 / sample rate (where 16777216 == 1s) and set divider to 0 == 1/1
                const uint32_t frameTime = uint64_t(16777216) / AUDIO_SAMPLERATE;
                REG_TM0CNT_L = 65536U - frameTime;
                REG_TM0CNT_H = 0; // start timer later
                // set timer 1 to cascade from timer 0, issue IRQ to swap sample buffers and set divider to 0 == 1/1
                Irq::setHandler(Irq::Mask::Timer1, AudioBufferRequest);
                Irq::enable(Irq::Mask::Timer1);
                REG_TM1CNT_L = 0;                           // set up number of samples in buffer later
                REG_TM1CNT_H = TIMER_COUNT | TIMER_IRQ | 0; // start timer later
                // read the first audio frame from media data
                while (m_queuedAudioFrame.data == nullptr)
                {
                    ReadAndQueueNextFrame();
                }
                // decode the first audio frame
                DecodeAudioFrame();
                // calculate time per sample in 0.32 format used in AudioBufferRequest()
                m_audioSampleTime = VIDEO_PLAYBACK_ON ? 0 : uint64_t(4294967296) / AUDIO_SAMPLERATE;
            }
            if (VIDEO_PLAYBACK_ON)
            {
                // fill scratch pad with clear color
                const uint32_t clearColor = static_cast<uint32_t>(m_vramClearColor) << 16 | m_vramClearColor;
                Memory::memset32(m_videoScratchPad, clearColor, m_videoScratchPadSize8 / 4);
                // set up timer 2 to increase with video frame interval = 1 / fps (where 65536 == 1s). frames/s are in 16:16 format
                // set timer 2 divider to 2 == 1/256 -> 16*1024*1024 cycles/s / 256 = 65536/s
                Irq::setHandler(Irq::Mask::Timer2, VideoFrameRequest);
                Irq::enable(Irq::Mask::Timer2);
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
            if (VIDEO_PLAYBACK_ON)
            {
                // blit first frame to screen
                BlitVideoFrame();
                // start timer for video frame rate
                REG_TM2CNT_H |= TIMER_START;
            }
            if (AUDIO_PLAYBACK_ON)
            {
                // start and enable sound playback
                // sound output will be enabled in AudioBufferRequest() to prevent clicks
                AudioBufferRequest();
            }
            if (SUBTITLES_PLAYBACK_ON && m_subtitlesEnabled)
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
        return m_playState != PlayState::Stopped && IO::Vid2h::HasMoreFrames(m_mediaInfo, m_mediaFrame);
    }

    IWRAM_FUNC auto DecodeAndPlay() -> void
    {
        if (m_playState == PlayState::Playing)
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
                // only decode the current frame if there are no more requests waiting, because this would mean we are skipping frames
                if (m_audioFramesRequested <= 0)
                {
                    DecodeAudioFrame();
                }
                else
                {
                    m_queuedAudioFrame.data = nullptr;
#ifdef DEBUG_VIDEOPLAYER
                    Debug::printf("Skipping audio frame #%d", m_queuedAudioFrame.index);
#endif
                }
            }
            if (m_videoFramesDecoded < 1 && m_queuedVideoFrame.data != nullptr)
            {
                --m_videoFramesRequested;
                // only decode the current frame if there are no more requests waiting, because this would mean we are skipping frames
                if (m_videoFramesRequested <= 0)
                {
                    DecodeVideoFrame();
                }
                else
                {
                    m_queuedVideoFrame.data = nullptr;
#ifdef DEBUG_VIDEOPLAYER
                    Debug::printf("Skipping video frame #%d", m_queuedVideoFrame.index);
#endif
                }
            }
            if (m_queuedSubtitleFrame.data != nullptr)
            {
                DecodeSubtitlesFrame();
            }
        }
    }

    auto GetPlayState() -> PlayState
    {
        return m_playState;
    }

    auto Pause(bool pause) -> void
    {
        if (pause)
        {
            if (m_playState == PlayState::Playing)
            {
                m_playState = PlayState::Paused;
                if (AUDIO_PLAYBACK_ON)
                {
                    // disable audio timers
                    REG_TM1CNT_H &= ~TIMER_START;
                    REG_TM0CNT_H &= ~TIMER_START;
                    // disable sound for now
                    REG_SOUNDCNT_X = 0;
                    REG_SOUNDBIAS = 0;
                }
                if (VIDEO_PLAYBACK_ON)
                {
                    // disable video timer
                    REG_TM2CNT_H &= ~TIMER_START;
                }
            }
        }
        else
        {
            if (m_playState == PlayState::Paused)
            {
                if (AUDIO_PLAYBACK_ON)
                {
                    m_playState = PlayState::Playing;
                    // enable audio timers
                    REG_TM1CNT_H |= TIMER_START;
                    REG_TM0CNT_H |= TIMER_START;
                    // sound output will be enabled in AudioBufferRequest() to prevent clicks
                }
                if (VIDEO_PLAYBACK_ON)
                {
                    // enable video timer
                    REG_TM2CNT_H |= TIMER_START;
                }
            }
        }
    }

    auto Stop() -> void
    {
        if (m_playState != PlayState::Stopped)
        {
            m_playState = PlayState::Stopped;
            m_audioFramesRequested = 0;
            m_videoFramesRequested = 0;
            // clean up audio
            if (AUDIO_PLAYBACK_ON)
            {
                // disable sound for now
                REG_SOUNDCNT_X = 0;
                REG_SOUNDBIAS = 0;
                // disable audio timers
                REG_TM1CNT_H = 0;
                REG_TM0CNT_H = 0;
                Irq::disable(Irq::Mask::Timer1);
                // disable audio DMAs
                REG_DMA[1].control = 0;
                if (m_mediaInfo.audio.channels == 2)
                {
                    REG_DMA[2].control = 0;
                }
                // free audio memory
                if (m_audioSampleBuffer[0])
                {
                    Memory::free(m_audioSampleBuffer[0]);
                    m_audioSampleBuffer[0] = nullptr;
                    m_audioSampleBuffer[1] = nullptr;
                    m_audioPlayPtr[0] = nullptr;
                    m_audioPlayPtr[1] = nullptr;
                    m_audioBackPtr[0] = nullptr;
                    m_audioBackPtr[1] = nullptr;
                }
                if (m_audioScratchPad)
                {
                    Memory::free(m_audioScratchPad);
                    m_audioScratchPad = nullptr;
                }
            }
            // clean up video
            if (VIDEO_PLAYBACK_ON)
            {
                // disable video timer
                REG_TM2CNT_H = 0;
                Irq::disable(Irq::Mask::Timer2);
                // free memory
                if (m_videoScratchPad)
                {
                    Memory::free(m_videoScratchPad);
                    m_videoScratchPad = nullptr;
                }
            }
            // clean up subtitles
            if (SUBTITLES_PLAYBACK_ON)
            {
                Subtitles::Cleanup();
                m_subtitleCurrent = {0, 0, nullptr};
                m_subtitleNext = {0, 0, nullptr};
            }
            // clean up video help
            if (HAS_FEATURE_HELP)
            {
                MediaHelp::Cleanup();
            }
#ifdef DEBUG_VIDEOPLAYER
            Debug::printf("Audio avg. decode: %.2f ms (max. %.2f ms)", int32_t(m_audioStats.m_accDecodedMs / m_audioStats.m_nrDecoded), m_audioStats.m_maxDecodedMs);
            Debug::printf("Video avg. decode: %.3f ms (max. %.3f ms)", int32_t(m_videoStats.m_accDecodedMs / m_videoStats.m_nrDecoded), m_videoStats.m_maxDecodedMs);
            Debug::printf("Video avg. blit: %.4f ms (max. %.4f ms)", int32_t(m_videoStats.m_accCopiedMs / m_videoStats.m_nrCopied), m_videoStats.m_maxCopiedMs);
            Debug::printf("Subtitles avg. update: %.1f ms (max. %.1f ms)", int32_t(m_subtitlesStats.m_accCopiedMs / m_subtitlesStats.m_nrCopied), m_subtitlesStats.m_maxCopiedMs);
#endif
        }
    }
}

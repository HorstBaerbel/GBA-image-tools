#pragma once

#include "color/xrgb8888.h"
#include "io/mediareader.h"
#include "timing.h"
#include "ui/sdlwindow.h"

#include <memory>
#include <string>
#include <vector>

namespace Media
{

    class Window : public Ui::SDLWindow
    {
    public:
        using SPtr = std::shared_ptr<Window>;

        /// @brief Play state of window
        enum PlayState
        {
            Stopped = 0,
            Playing = 1,
            Paused = 2
        };

        Window(uint32_t width, uint32_t height, const std::string &title);
        ~Window();

        auto getPlayState() const -> PlayState;

        auto play(std::shared_ptr<Reader> mediaReader) -> void;
        auto pause(bool pause = true) -> void;
        auto stop() -> void;

    private:
        auto quitEvent(SDL_Event event) -> bool override;
        auto userEvent(SDL_Event event) -> int override;
        auto displayEvent(void *data) -> void;

        auto readFrames() -> void;

        std::deque<std::vector<uint8_t>> m_audioData;
        uint32_t m_audioFrameIndex = 0;
        std::deque<Image::RawData> m_videoData;
        uint32_t m_videoFrameIndex = 0;
        std::deque<Subtitles::RawData> m_subtitlesData;
        uint32_t m_subtitlesFrameIndex = 0;
        std::vector<Subtitles::RawData> m_currentSubtitles;

        SDL_AudioStream *m_sdlAudioStream = nullptr;
        SDL_Texture *m_sdlVideoTexture = nullptr;

        std::shared_ptr<Reader> m_mediaReader;
        Reader::MediaInfo m_mediaInfo;
        double m_frameIntervalMs = 0;
        Timer m_frameTimer;
        double m_playTimeS = 0;
        PlayState m_playState = PlayState::Stopped;

        static constexpr const int32_t EVENT_DISPLAY_FRAME = 1;
        static constexpr const int32_t EVENT_STOP = 2;
    };

}

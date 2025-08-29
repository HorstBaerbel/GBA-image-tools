#include "mediawindow.h"

#include "exception.h"

#include <SDL_audio.h>

#include <cstring>

namespace Media
{

    Window::Window(uint32_t width, uint32_t height, const std::string &title)
        : SDLWindow(width, height, title)
    {
        SDL_InitSubSystem(SDL_INIT_AUDIO);
    }

    Window::~Window()
    {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }

    auto Window::getPlayState() const -> Window::PlayState
    {
        return m_playState;
    }

    auto Window::play(std::shared_ptr<Reader> mediaReader) -> void
    {
        lockEventMutex();
        if (m_playState == PlayState::Stopped)
        {
            m_playState = PlayState::Playing;
            m_audioFrameIndex = 0;
            m_videoFrameIndex = 0;
            m_mediaReader = mediaReader;
            m_mediaInfo = m_mediaReader->getInfo();
            REQUIRE(m_mediaInfo.fileType & IO::FileType::Audio == 0 || m_mediaInfo.audioNrOfFrames > 0, std::runtime_error, "Audio file, but no audio frames");
            REQUIRE(m_mediaInfo.fileType & IO::FileType::Video == 0 || m_mediaInfo.videoNrOfFrames > 0, std::runtime_error, "Video file, but no video frames");
            // read first frames
            int32_t requestedAudioFrames = m_mediaInfo.fileType & IO::FileType::Audio != 0 && m_mediaInfo.audioNrOfFrames > m_audioFrameIndex ? 1 : 0;
            int32_t requestedVideoFrames = m_mediaInfo.fileType & IO::FileType::Video != 0 && m_mediaInfo.videoNrOfFrames > m_videoFrameIndex ? 1 : 0;
            while (requestedAudioFrames > 0 || requestedVideoFrames > 0)
            {
                auto frame = m_mediaReader->readFrame();
                if (frame.frameType == IO::FrameType::Audio)
                {
                    ++m_audioFrameIndex;
                    --requestedAudioFrames;
                    m_audioData.push_back(std::get<std::vector<int16_t>>(frame.data));
                }
                else if (frame.frameType == IO::FrameType::Pixels)
                {
                    ++m_videoFrameIndex;
                    --requestedVideoFrames;
                    m_videoData.push_back(std::get<std::vector<Color::XRGB8888>>(frame.data));
                }
            }
            // start frame timer
            const auto interval = 1000.0 / m_mediaInfo.videoFrameRateHz;
            m_frameTimer.start(interval, [this]()
                               { displayEvent(nullptr); });
        }
        unlockEventMutex();
    }

    auto Window::pause(bool pause) -> void
    {
        lockEventMutex();
        if (pause)
        {
            if (m_playState == PlayState::Playing)
            {
                m_playState == PlayState::Paused;
            }
        }
        else
        {
            if (m_playState == PlayState::Paused)
            {
                m_playState == PlayState::Playing;
            }
        }
        unlockEventMutex();
    }

    auto Window::stop() -> void
    {
        lockEventMutex();
        if (m_playState != PlayState::Stopped)
        {
            m_playState = PlayState::Stopped;
            m_frameTimer.stop();
        }
        unlockEventMutex();
    }

    auto Window::quitEvent(SDL_Event event) -> bool
    {
        lockEventMutex();
        stop();
        if (m_sdlTexture != nullptr)
        {
            SDL_DestroyTexture(m_sdlTexture);
            m_sdlTexture = nullptr;
        }
        unlockEventMutex();
        return true;
    }

    auto Window::userEvent(SDL_Event event) -> void
    {
        // lock to check if data from other thread
        lockEventMutex();
        // check if we need to allocate a texture
        if (m_sdlTexture == nullptr)
        {
            m_sdlTexture = SDL_CreateTexture(getRenderer(), SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, m_mediaInfo.videoWidth, m_mediaInfo.videoHeight);
            if (m_sdlTexture == nullptr)
            {
                unlockEventMutex();
                return;
            }
        }
        // check if if we want to display a frame
        if (event.user.code == EVENT_DISPLAY_FRAME)
        {
            // check if we have video / audio data pending
            if (!m_videoData.empty())
            {
                // we have data, get it, remove from the queue and unlock
                auto image = m_videoData.front();
                m_videoData.pop_front();
                unlockEventMutex();
                // process data
                if (m_sdlTexture != nullptr && image.size() == m_mediaInfo.videoWidth * m_mediaInfo.videoHeight)
                {
                    // SDL_Rect dstRect = {data.x, data.y, static_cast<int>(data.width), static_cast<int>(data.height)};
                    void *pixels = nullptr;
                    int pitch = 0;
                    auto lockResult = SDL_LockTexture(m_sdlTexture, nullptr, &pixels, &pitch);
                    if (lockResult == 0 && pixels != nullptr && pitch != 0)
                    {
                        std::memcpy(pixels, image.data(), image.size() * sizeof(Color::XRGB8888));
                        SDL_UnlockTexture(m_sdlTexture);
                    }
                    SDL_RenderCopy(getRenderer(), m_sdlTexture, nullptr, nullptr); //&dstRect);
                    SDL_RenderPresent(getRenderer());
                }
            }
            // else: we're skipping frames...
            // check if we can stop playing
            // ???
        }
        else
        {
            unlockEventMutex();
        }
    }

    auto Window::displayEvent(void *data) -> void
    {
        lockEventMutex();
        if (m_playState == PlayState::Playing)
        {
            // push event to display frame
            pushUserEvent(EVENT_DISPLAY_FRAME);
            // check if we have already reached the end of the media file
            if (m_mediaInfo.audioNrOfFrames <= m_audioFrameIndex && m_mediaInfo.videoNrOfFrames <= m_videoFrameIndex)
            {
                unlockEventMutex();
                stop();
            }
            // read next frames
            int32_t requestedAudioFrames = m_mediaInfo.fileType & IO::FileType::Audio != 0 && m_mediaInfo.audioNrOfFrames > m_audioFrameIndex ? 1 : 0;
            int32_t requestedVideoFrames = m_mediaInfo.fileType & IO::FileType::Video != 0 && m_mediaInfo.videoNrOfFrames > m_videoFrameIndex ? 1 : 0;
            while (requestedAudioFrames > 0 || requestedVideoFrames > 0)
            {
                auto frame = m_mediaReader->readFrame();
                if (frame.frameType == IO::FrameType::Audio)
                {
                    ++m_audioFrameIndex;
                    --requestedAudioFrames;
                    m_audioData.push_back(std::get<std::vector<int16_t>>(frame.data));
                }
                else if (frame.frameType == IO::FrameType::Pixels)
                {
                    ++m_videoFrameIndex;
                    --requestedVideoFrames;
                    m_videoData.push_back(std::get<std::vector<Color::XRGB8888>>(frame.data));
                }
            }
        }
        unlockEventMutex();
    }
}

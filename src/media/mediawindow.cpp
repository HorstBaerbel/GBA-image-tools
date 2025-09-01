#include "mediawindow.h"

#include "audio/audiohelpers.h"
#include "exception.h"

#include <SDL3/SDL_audio.h>

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
            const bool hasAudio = m_mediaInfo.fileType & IO::FileType::Audio != 0;
            REQUIRE(!hasAudio || m_mediaInfo.audioNrOfFrames > 0, std::runtime_error, "Audio file, but no audio frames");
            const bool hasVideo = m_mediaInfo.fileType & IO::FileType::Video != 0;
            REQUIRE(!hasVideo || m_mediaInfo.videoNrOfFrames > 0, std::runtime_error, "Video file, but no video frames");
            // open audio device in paused state
            if (hasAudio)
            {
                SDL_AudioSpec audioSpec;
                audioSpec.channels = Audio::formatInfo(m_mediaInfo.audioChannelFormat).nrOfChannels;
                audioSpec.format = SDL_AUDIO_S16LE;
                audioSpec.freq = static_cast<int>(m_mediaInfo.audioSampleRateHz);
                m_sdlAudioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audioSpec, nullptr, nullptr);
                REQUIRE(m_sdlAudioStream != nullptr, std::runtime_error, "Failed to create SDL audio stream: " << SDL_GetError());
            }
            // read first audio and video frames
            readFrames();
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
                SDL_PauseAudioStreamDevice(m_sdlAudioStream);
            }
        }
        else
        {
            if (m_playState == PlayState::Paused)
            {
                m_playState == PlayState::Playing;
                SDL_ResumeAudioStreamDevice(m_sdlAudioStream);
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
            SDL_PauseAudioStreamDevice(m_sdlAudioStream);
            SDL_ClearAudioStream(m_sdlAudioStream);
            m_frameTimer.stop();
        }
        if (m_sdlAudioStream != nullptr)
        {
            SDL_DestroyAudioStream(m_sdlAudioStream);
            m_sdlAudioStream = nullptr;
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

    auto Window::userEvent(SDL_Event event) -> int
    {
        // check if we want to stop playback
        if (event.user.code == EVENT_STOP)
        {
            stop();
        }
        // lock to check if data from other thread
        lockEventMutex();
        // check if if we want to display a frame
        if (event.user.code == EVENT_DISPLAY_FRAME)
        {
            // check if we have video / audio data pending
            if (!m_audioData.empty())
            {
                // we have audio data, get it, remove from the queue
                auto samples = m_audioData.front();
                m_audioData.pop_front();
                // copy data to stream and play
                SDL_PutAudioStreamData(m_sdlAudioStream, samples.data(), samples.size());
                SDL_ResumeAudioStreamDevice(m_sdlAudioStream);
            }
            if (!m_videoData.empty())
            {
                // we have video data, get it, remove from the queue
                auto image = m_videoData.front();
                m_videoData.pop_front();
                // check if we need to allocate a texture
                if (m_sdlTexture == nullptr)
                {
                    m_sdlTexture = SDL_CreateTexture(getRenderer(), SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, m_mediaInfo.videoWidth, m_mediaInfo.videoHeight);
                    if (m_sdlTexture == nullptr)
                    {
                        std::cerr << "Failed to create SDL texture: " << SDL_GetError() << std::endl;
                        unlockEventMutex();
                        return -1;
                    }
                    if (!SDL_SetTextureScaleMode(m_sdlTexture, SDL_SCALEMODE_NEAREST))
                    {
                        std::cerr << "Failed to set SDL texture scale mode: " << SDL_GetError() << std::endl;
                    }
                }
                // copy data to texture and render
                if (m_sdlTexture != nullptr && image.size() == m_mediaInfo.videoWidth * m_mediaInfo.videoHeight)
                {
                    void *pixels = nullptr;
                    int pitch = 0;
                    auto lockResult = SDL_LockTexture(m_sdlTexture, nullptr, &pixels, &pitch);
                    if (lockResult && pixels != nullptr && pitch != 0)
                    {
                        std::memcpy(pixels, image.data(), image.size() * sizeof(Color::XRGB8888));
                        SDL_UnlockTexture(m_sdlTexture);
                    }
                    if (!SDL_RenderTexture(getRenderer(), m_sdlTexture, nullptr, nullptr))
                    {
                        std::cerr << "Failed to render SDL texture: " << SDL_GetError() << std::endl;
                        unlockEventMutex();
                        return -1;
                    }
                    if (!SDL_RenderPresent(getRenderer()))
                    {
                        std::cerr << "Failed to present SDL render: " << SDL_GetError() << std::endl;
                        unlockEventMutex();
                        return -1;
                    }
                }
            }
            // else: we're skipping frames...
        }
        unlockEventMutex();
        return 0;
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
                // push event to stop playback
                pushUserEvent(EVENT_STOP);
                unlockEventMutex();
            }
            else
            {
                // read next audio and video frames
                readFrames();
            }
        }
        unlockEventMutex();
    }

    auto Window::readFrames() -> void
    {
        int32_t requestedAudioFrames = m_mediaInfo.fileType & IO::FileType::Audio != 0 && m_mediaInfo.audioNrOfFrames > m_audioFrameIndex ? 1 : 0;
        int32_t requestedVideoFrames = m_mediaInfo.fileType & IO::FileType::Video != 0 && m_mediaInfo.videoNrOfFrames > m_videoFrameIndex ? 1 : 0;
        while (requestedAudioFrames > 0 || requestedVideoFrames > 0)
        {
            auto frame = m_mediaReader->readFrame();
            if (frame.frameType == IO::FrameType::Audio)
            {
                ++m_audioFrameIndex;
                --requestedAudioFrames;
                const auto &planarSamples = std::get<std::vector<int16_t>>(frame.data);
                m_audioData.push_back(AudioHelpers::toRawInterleavedData(planarSamples, m_mediaInfo.audioChannelFormat));
            }
            else if (frame.frameType == IO::FrameType::Pixels)
            {
                ++m_videoFrameIndex;
                --requestedVideoFrames;
                m_videoData.push_back(std::get<std::vector<Color::XRGB8888>>(frame.data));
            }
        }
    }
}

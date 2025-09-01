#include "statisticswindow.h"

#include <iostream>

namespace Statistics
{

    Window::Window(uint32_t width, uint32_t height, const std::string &title)
        : SDLWindow(width, height, title), m_container(std::make_shared<Container>())
    {
    }

    auto Window::getStatisticsContainer() -> Container::SPtr
    {
        return m_container;
    }

    auto Window::displayImage(const std::vector<uint8_t> &data, Ui::ColorFormat format, uint32_t width, uint32_t height, int32_t x, int32_t y) -> void
    {
        if (isActive())
        {
            // copy data to thread event queue
            lockEventMutex();
            m_eventData.emplace_back(DisplayImage{format, data, width, height, x, y});
            unlockEventMutex();
            // notify SDL thread about event
            pushUserEvent();
        }
    }

    auto Window::displayImage(const uint8_t *data, std::size_t size, Ui::ColorFormat format, uint32_t width, uint32_t height, int32_t x, int32_t y) -> void
    {
        if (isActive())
        {
            SDL_assert(data != nullptr);
            SDL_assert(size != 0);
            // copy data to thread event queue
            lockEventMutex();
            m_eventData.emplace_back(DisplayImage{format, std::vector<uint8_t>(data, data + size), width, height, x, y});
            unlockEventMutex();
            // notify SDL thread about event
            pushUserEvent();
        }
    }

    auto Window::update() -> void
    {
        auto frames = m_container->getFrames();
        if (!frames.empty())
        {
            auto images = frames.back()->getImages();
            for (const auto &image : images)
            {
                const auto &data = image.second;
                switch (data.colorFormat)
                {
                case Color::Format::XRGB1555:
                    displayImage(data.image, Ui::ColorFormat::XRGB1555, data.width, data.height);
                    break;
                case Color::Format::RGB565:
                    displayImage(data.image, Ui::ColorFormat::RGB565, data.width, data.height);
                    break;
                case Color::Format::XBGR1555:
                    displayImage(data.image, Ui::ColorFormat::XBGR1555, data.width, data.height);
                    break;
                case Color::Format::BGR565:
                    displayImage(data.image, Ui::ColorFormat::BGR565, data.width, data.height);
                    break;
                case Color::Format::XRGB8888:
                    displayImage(data.image, Ui::ColorFormat::XRGB8888, data.width, data.height);
                    break;
                default:
                    THROW(std::runtime_error, "Unsupported color format");
                }
            }
        }
    }

    auto Window::quitEvent(SDL_Event event) -> bool
    {
        lockEventMutex();
        if (m_sdlTexture != nullptr)
        {
            SDL_DestroyTexture(m_sdlTexture);
            m_sdlTexture = nullptr;
            m_sdlTexturePixelFormat = SDL_PIXELFORMAT_UNKNOWN;
            m_sdlTextureWidth = 0;
            m_sdlTextureHeight = 0;
        }
        unlockEventMutex();
        return true;
    }

    auto Window::userEvent(SDL_Event event) -> int
    {
        // lock to check if data from other thread
        lockEventMutex();
        if (!m_eventData.empty())
        {
            // we have data, get it, remove from the queue and unlock
            auto eventData = m_eventData.front();
            m_eventData.pop_front();
            // process data
            if (std::holds_alternative<DisplayImage>(eventData))
            {
                const auto &data = std::get<DisplayImage>(eventData);
                SDL_PixelFormat requiredPixelFormat = SDL_PIXELFORMAT_UNKNOWN;
                switch (data.format)
                {
                case Ui::ColorFormat::XRGB1555:
                    requiredPixelFormat = SDL_PIXELFORMAT_XRGB1555;
                    break;
                case Ui::ColorFormat::RGB565:
                    requiredPixelFormat = SDL_PIXELFORMAT_RGB565;
                    break;
                case Ui::ColorFormat::XBGR1555:
                    requiredPixelFormat = SDL_PIXELFORMAT_XBGR1555;
                    break;
                case Ui::ColorFormat::BGR565:
                    requiredPixelFormat = SDL_PIXELFORMAT_BGR565;
                    break;
                case Ui::ColorFormat::XRGB8888:
                    requiredPixelFormat = SDL_PIXELFORMAT_XRGB8888;
                    break;
                default:
                    std::cerr << "Unkown data pixel format: " << static_cast<int>(data.format) << std::endl;
                    return -1;
                }
                // check if we need to (re-)allocate the texture
                if (m_sdlTexture == nullptr || m_sdlTexturePixelFormat != requiredPixelFormat || m_sdlTextureWidth != data.width || m_sdlTextureHeight != data.height)
                {
                    if (m_sdlTexture != nullptr)
                    {
                        SDL_DestroyTexture(m_sdlTexture);
                        m_sdlTexture = nullptr;
                        m_sdlTexturePixelFormat = SDL_PIXELFORMAT_UNKNOWN;
                        m_sdlTextureWidth = 0;
                        m_sdlTextureHeight = 0;
                    }
                    m_sdlTexture = SDL_CreateTexture(getRenderer(), requiredPixelFormat, SDL_TEXTUREACCESS_STREAMING, data.width, data.height);
                    if (m_sdlTexture == nullptr)
                    {
                        std::cerr << "Failed to create SDL texture: " << SDL_GetError() << std::endl;
                        unlockEventMutex();
                        return -1;
                    }
                    m_sdlTexturePixelFormat = requiredPixelFormat;
                    m_sdlTextureWidth = data.width;
                    m_sdlTextureHeight = data.height;
                    if (!SDL_SetTextureScaleMode(m_sdlTexture, SDL_SCALEMODE_NEAREST))
                    {
                        std::cerr << "Failed to set SDL texture scale mode: " << SDL_GetError() << std::endl;
                    }
                }
                // update texture
                void *pixels = nullptr;
                int pitch = 0;
                auto lockResult = SDL_LockTexture(m_sdlTexture, nullptr, &pixels, &pitch);
                if (lockResult && pixels != nullptr && pitch != 0)
                {
                    std::memcpy(pixels, data.image.data(), data.image.size());
                    SDL_UnlockTexture(m_sdlTexture);
                }
                // render texture
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
        unlockEventMutex();
        return 0;
    }
}

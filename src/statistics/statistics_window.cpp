#include "statistics_window.h"

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

    auto Window::userEvent(SDL_Event event) -> void
    {
        // lock to check if data from other thread
        lockEventMutex();
        if (!m_eventData.empty())
        {
            // we have data, get it, remove from the queue and unlock
            auto eventData = m_eventData.front();
            m_eventData.pop_front();
            unlockEventMutex();
            // process data
            if (std::holds_alternative<DisplayImage>(eventData))
            {
                SDL_Surface *surface = nullptr;
                const auto &data = std::get<DisplayImage>(eventData);
                switch (data.format)
                {
                case Ui::ColorFormat::XRGB1555:
                    surface = SDL_CreateRGBSurfaceWithFormat(0, data.width, data.height, 15, SDL_PIXELFORMAT_RGB555);
                    break;
                case Ui::ColorFormat::RGB565:
                    surface = SDL_CreateRGBSurfaceWithFormat(0, data.width, data.height, 16, SDL_PIXELFORMAT_RGB565);
                    break;
                case Ui::ColorFormat::XBGR1555:
                    surface = SDL_CreateRGBSurfaceWithFormat(0, data.width, data.height, 15, SDL_PIXELFORMAT_BGR555);
                    break;
                case Ui::ColorFormat::BGR565:
                    surface = SDL_CreateRGBSurfaceWithFormat(0, data.width, data.height, 16, SDL_PIXELFORMAT_BGR565);
                    break;
                case Ui::ColorFormat::XRGB8888:
                    surface = SDL_CreateRGBSurfaceWithFormat(0, data.width, data.height, 32, SDL_PIXELFORMAT_XRGB8888);
                    break;
                }
                if (surface == nullptr)
                {
                    return;
                }
                std::memcpy(surface->pixels, data.image.data(), data.image.size());
                auto *texture = SDL_CreateTextureFromSurface(getRenderer(), surface);
                if (texture == nullptr)
                {
                    SDL_FreeSurface(surface);
                    return;
                }
                // SDL_Rect dstRect = {data.x, data.y, static_cast<int>(data.width), static_cast<int>(data.height)};
                SDL_RenderCopy(getRenderer(), texture, nullptr, nullptr); //&dstRect);
                SDL_RenderPresent(getRenderer());
                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
            }
        }
        else
        {
            unlockEventMutex();
        }
    }
}

#include "gui_sdl.h"

#include <SDL_video.h>

#include <cstring>
#include <stdexcept>
#include <iostream>

namespace Ui
{

    SDLWindow::SDLWindow(uint32_t width, uint32_t height)
        : m_width(width), m_height(height)
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
        m_mutex = SDL_CreateMutex();
        if (m_mutex == nullptr)
        {
            throw std::runtime_error(SDL_GetError());
        }
        m_thread = SDL_CreateThread(MessageLoop, "SDL message loop", this);
        if (m_thread == nullptr)
        {
            throw std::runtime_error(SDL_GetError());
        }
    }

    SDLWindow::~SDLWindow()
    {
        m_quit = true;
        if (m_thread != nullptr)
        {
            SDL_WaitThread(m_thread, nullptr);
        }
        if (m_mutex != nullptr)
        {
            SDL_DestroyMutex(m_mutex);
        }
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    auto SDLWindow::MessageLoop(void *object) -> int
    {
        SDLWindow *w = reinterpret_cast<SDLWindow *>(object);
        if (w == nullptr)
        {
            std::cerr << "Bad object pointer" << std::endl;
            return -1;
        }
        SDL_Window *sdlWindow = SDL_CreateWindow("vid2h", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w->m_width, w->m_height, SDL_WINDOW_VULKAN);
        if (sdlWindow == nullptr)
        {
            std::cerr << "Failed to create SDL window" << std::endl;
            return -2;
        }
        SDL_Renderer *renderer = SDL_CreateRenderer(sdlWindow, -1, 0);
        if (renderer == nullptr)
        {
            std::cerr << "Failed to create SDL renderer" << std::endl;
            SDL_DestroyWindow(sdlWindow);
            return -3;
        }
        SDL_Event event;
        while (!w->m_quit)
        {
            while (SDL_PollEvent(&event) != 0)
            {
                switch (event.type)
                {
                case SDL_QUIT:
                    w->m_quit = true;
                    break;
                case SDL_USEREVENT:
                    // lock to check if data from other thread
                    SDL_LockMutex(w->m_mutex);
                    if (!w->m_eventData.empty())
                    {
                        // we have data, get it, remove from the queue and unlock
                        auto eventData = w->m_eventData.front();
                        w->m_eventData.pop_front();
                        SDL_UnlockMutex(w->m_mutex);
                        // process data
                        if (std::holds_alternative<DisplayImage>(eventData))
                        {
                            SDL_Surface *surface = nullptr;
                            const auto &data = std::get<DisplayImage>(eventData);
                            switch (data.format)
                            {
                            case ColorFormat::FormatXRGB8888:
                                surface = SDL_CreateRGBSurfaceWithFormat(0, data.width, data.height, 24, SDL_PIXELFORMAT_RGB24);
                                break;
                            case ColorFormat::FormatXRGB1555:
                                surface = SDL_CreateRGBSurfaceWithFormat(0, data.width, data.height, 15, SDL_PIXELFORMAT_RGB555);
                                break;
                            }
                            if (surface == nullptr)
                            {
                                break;
                            }
                            std::memcpy(surface->pixels, data.image.data(), data.image.size());
                            auto *texture = SDL_CreateTextureFromSurface(renderer, surface);
                            if (surface == nullptr)
                            {
                                SDL_FreeSurface(surface);
                                break;
                            }
                            // SDL_Rect dstRect = {data.x, data.y, static_cast<int>(data.width), static_cast<int>(data.height)};
                            SDL_RenderCopy(renderer, texture, nullptr, nullptr); //&dstRect);
                            SDL_RenderPresent(renderer);
                            SDL_DestroyTexture(texture);
                            SDL_FreeSurface(surface);
                        }
                    }
                    else
                    {
                        SDL_UnlockMutex(w->m_mutex);
                    }
                    break;
                }
            }
            SDL_Delay(0);
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(sdlWindow);
        return 0;
    }

    auto SDLWindow::displayImageXRGB8888(const std::vector<uint8_t> &image, uint32_t width, uint32_t height, int32_t x, int32_t y) -> void
    {
        if (!m_quit)
        {
            // copy data to thread event queue
            SDL_LockMutex(m_mutex);
            m_eventData.emplace_back(DisplayImage{ColorFormat::FormatXRGB8888, image, width, height, x, y});
            SDL_UnlockMutex(m_mutex);
            // notify SDL thread about event
            SDL_Event e;
            e.type = SDL_USEREVENT;
            e.user.code = 0;
            e.user.data1 = nullptr;
            e.user.data2 = nullptr;
            SDL_PushEvent(&e);
        }
    }

    auto SDLWindow::displayImageXRGB1555(const std::vector<uint8_t> &image, uint32_t width, uint32_t height, int32_t x, int32_t y) -> void
    {
        if (!m_quit)
        {
            // copy data to thread event queue
            SDL_LockMutex(m_mutex);
            m_eventData.emplace_back(DisplayImage{ColorFormat::FormatXRGB1555, image, width, height, x, y});
            SDL_UnlockMutex(m_mutex);
            // notify SDL thread about event
            SDL_Event e;
            e.type = SDL_USEREVENT;
            e.user.code = 0;
            e.user.data1 = nullptr;
            e.user.data2 = nullptr;
            SDL_PushEvent(&e);
        }
    }
}
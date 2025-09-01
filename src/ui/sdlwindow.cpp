#include "sdlwindow.h"

#include "exception.h"

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_video.h>

#include <cstring>
#include <iostream>
#include <stdexcept>

namespace Ui
{

    SDLWindow::SDLWindow(uint32_t width, uint32_t height, const std::string &title)
        : m_width(width), m_height(height), m_title(title)
    {
        REQUIRE(SDL_InitSubSystem(SDL_INIT_VIDEO), std::runtime_error, "Failed to init SDL video: " << SDL_GetError());
        m_eventMutex = SDL_CreateMutex();
        REQUIRE(m_eventMutex != nullptr, std::runtime_error, "Failed to create SDL mutex: " << SDL_GetError());
        m_msgLoopThread = SDL_CreateThread(MessageLoop, "SDL message loop", this);
        if (m_msgLoopThread == nullptr)
        {
            const auto createError = SDL_GetError();
            SDL_DestroyMutex(m_eventMutex);
            m_eventMutex = nullptr;
            THROW(std::runtime_error, "Failed to create SDL thread: " << createError);
        }
    }

    SDLWindow::~SDLWindow()
    {
        m_quit = true;
        if (m_msgLoopThread != nullptr)
        {
            SDL_WaitThread(m_msgLoopThread, nullptr);
            m_msgLoopThread = nullptr;
        }
        if (m_eventMutex != nullptr)
        {
            SDL_DestroyMutex(m_eventMutex);
            m_eventMutex = nullptr;
        }
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    auto SDLWindow::isActive() const -> bool
    {
        return !m_quit;
    }

    auto SDLWindow::quitEvent(SDL_Event event) -> bool
    {
        return true;
    }

    auto SDLWindow::pushUserEvent(int32_t code, void *data1, void *data2) -> void
    {
        SDL_Event e;
        e.type = SDL_EVENT_USER;
        e.user.code = code;
        e.user.data1 = data1;
        e.user.data2 = data2;
        SDL_PushEvent(&e);
    }

    auto SDLWindow::getRenderer() const -> SDL_Renderer *
    {
        return m_sdlRenderer;
    }

    auto SDLWindow::getWindow() const -> SDL_Window *
    {
        return m_sdlWindow;
    }

    auto SDLWindow::lockEventMutex() -> void
    {
        SDL_LockMutex(m_eventMutex);
    }

    auto SDLWindow::unlockEventMutex() -> void
    {
        SDL_UnlockMutex(m_eventMutex);
    }

    auto SDLWindow::MessageLoop(void *object) -> int
    {
        SDLWindow *w = reinterpret_cast<SDLWindow *>(object);
        if (w == nullptr)
        {
            std::cerr << "Bad object pointer in message loop" << std::endl;
            return -1;
        }
        w->m_sdlWindow = SDL_CreateWindow(w->m_title.data(), w->m_width, w->m_height, SDL_WINDOW_VULKAN);
        if (w->m_sdlWindow == nullptr)
        {
            std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
            return -1;
        }
        w->m_sdlRenderer = SDL_CreateRenderer(w->m_sdlWindow, nullptr);
        if (w->m_sdlRenderer == nullptr)
        {
            std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(w->m_sdlWindow);
            return -1;
        }
        // initially present render. this might be needed on Wayland systems
        if (!SDL_RenderPresent(w->m_sdlRenderer))
        {
            std::cerr << "Initial SDL render present failed: " << SDL_GetError() << std::endl;
            return -1;
        }
        SDL_Event event;
        while (!w->m_quit)
        {
            while (SDL_PollEvent(&event) != 0)
            {
                switch (event.type)
                {
                case SDL_EVENT_QUIT:
                    w->m_quit = w->quitEvent(event);
                    break;
                case SDL_EVENT_USER:
                    if (const auto eventResult = w->userEvent(event); eventResult != 0)
                    {
                        w->quitEvent(event);
                        w->m_quit = true;
                    }
                    break;
                }
            }
            SDL_Delay(1);
        }
        SDL_DestroyRenderer(w->m_sdlRenderer);
        w->m_sdlRenderer = nullptr;
        SDL_DestroyWindow(w->m_sdlWindow);
        w->m_sdlWindow = nullptr;
        return 0;
    }
}

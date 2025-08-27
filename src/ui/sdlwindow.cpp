#include "sdlwindow.h"

#include <SDL_video.h>

#include <cstring>
#include <iostream>
#include <stdexcept>

namespace Ui
{

    SDLWindow::SDLWindow(uint32_t width, uint32_t height, const std::string &title)
        : m_width(width), m_height(height), m_title(title)
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
        m_eventMutex = SDL_CreateMutex();
        if (m_eventMutex == nullptr)
        {
            throw std::runtime_error(SDL_GetError());
        }
        m_msgLoopThread = SDL_CreateThread(MessageLoop, "SDL message loop", this);
        if (m_msgLoopThread == nullptr)
        {
            throw std::runtime_error(SDL_GetError());
        }
    }

    SDLWindow::~SDLWindow()
    {
        m_quit = true;
        if (m_msgLoopThread != nullptr)
        {
            SDL_WaitThread(m_msgLoopThread, nullptr);
        }
        if (m_eventMutex != nullptr)
        {
            SDL_DestroyMutex(m_eventMutex);
        }
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    auto SDLWindow::isActive() const -> bool
    {
        return !m_quit;
    }

    auto SDLWindow::pushUserEvent(int32_t code, void *data1, void *data2) -> void
    {
        SDL_Event e;
        e.type = SDL_USEREVENT;
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
            std::cerr << "Bad object pointer" << std::endl;
            return -1;
        }
        w->m_sdlWindow = SDL_CreateWindow(w->m_title.data(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w->m_width, w->m_height, SDL_WINDOW_VULKAN);
        if (w->m_sdlWindow == nullptr)
        {
            std::cerr << "Failed to create SDL window" << std::endl;
            return -2;
        }
        w->m_sdlRenderer = SDL_CreateRenderer(w->m_sdlWindow, -1, 0);
        if (w->m_sdlRenderer == nullptr)
        {
            std::cerr << "Failed to create SDL renderer" << std::endl;
            SDL_DestroyWindow(w->m_sdlWindow);
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
                    w->m_quit = w->quitEvent(event);
                    break;
                case SDL_USEREVENT:
                    w->userEvent(event);
                    break;
                }
            }
            SDL_Delay(0);
        }
        SDL_DestroyRenderer(w->m_sdlRenderer);
        w->m_sdlRenderer = nullptr;
        SDL_DestroyWindow(w->m_sdlWindow);
        w->m_sdlWindow = nullptr;
        return 0;
    }
}

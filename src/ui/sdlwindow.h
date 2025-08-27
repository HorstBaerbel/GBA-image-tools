#pragma once

#include <SDL.h>

#include <atomic>
#include <deque>
#include <string>
#include <variant>

namespace Ui
{

    class SDLWindow
    {
    public:
        SDLWindow(uint32_t width, uint32_t height, const std::string &name = "");

        /// @brief Check if window is still active or has quit
        /// @return True if window is still active
        auto isActive() const -> bool;

        ~SDLWindow();

    protected:
        /// @brief Called when a quit event is received through the window system
        /// @param event Event data
        /// @return Pass true if you want to close the window, false if not
        virtual auto quitEvent(SDL_Event event) -> bool = 0;

        /// @brief Called when a user event is received through the window system
        /// @param event User event
        virtual auto userEvent(SDL_Event event) -> void = 0;

        /// @brief Push user event to message loop
        /// @param code User message code
        /// @param data1 User message data
        /// @param data2 User message data
        auto pushUserEvent(int32_t code = 0, void *data1 = nullptr, void *data2 = nullptr) -> void;

        /// @brief Get SDL renderer for drawing operations
        /// @return SDL renderer
        auto getRenderer() const -> SDL_Renderer *;

        /// @brief Get SDL window
        /// @return SDL window
        auto getWindow() const -> SDL_Window *;

        /// @brief Lock mutex used to exchange data from message loop thread to main thread
        auto lockEventMutex() -> void;

        /// @brief Unlock mutex used to exchange data from message loop thread to main thread
        auto unlockEventMutex() -> void;

    private:
        /// @brief SDL message loop receiving SDL events
        /// @param object Pointer to this object
        /// @return 0 == success, everything else is an error
        static auto MessageLoop(void *object) -> int;

        std::atomic<bool> m_quit = false;
        std::atomic<SDL_mutex *> m_eventMutex = nullptr;
        std::atomic<SDL_Thread *> m_msgLoopThread = nullptr;
        SDL_Window *m_sdlWindow = nullptr;
        SDL_Renderer *m_sdlRenderer = nullptr;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        std::string m_title;
    };

}
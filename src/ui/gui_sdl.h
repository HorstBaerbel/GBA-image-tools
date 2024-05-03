#pragma once

#include "gui.h"

#include <SDL.h>

#include <deque>
#include <variant>
#include <atomic>

namespace Ui
{

    class SDLWindow : public Window
    {
    public:
        SDLWindow(uint32_t width, uint32_t height);
        ~SDLWindow();

        auto displayImage(const std::vector<uint8_t> &image, ColorFormat format, uint32_t width, uint32_t height, int32_t x = 0, int32_t y = 0) -> void override;

    private:
        struct DisplayImage
        {
            ColorFormat format = ColorFormat::Unknown;
            std::vector<uint8_t> image;
            uint32_t width = 0;
            uint32_t height = 0;
            int32_t x = 0;
            int32_t y = 0;
        };

        static auto MessageLoop(void *object) -> int;

        std::atomic<bool> m_quit = false;
        std::atomic<SDL_mutex *> m_mutex = nullptr;
        std::atomic<SDL_Thread *> m_thread = nullptr;
        std::deque<std::variant<DisplayImage>> m_eventData;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
    };

}
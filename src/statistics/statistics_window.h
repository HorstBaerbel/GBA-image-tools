#pragma once

#include "statistics.h"
#include "ui/gui_sdl.h"

namespace Statistics
{

    class Window : public Ui::SDLWindow
    {
    public:
        Window(uint32_t width, uint32_t height);

        auto getStatisticsContainer() -> Container::SPtr;

        auto update() -> void;

    private:
        Container::SPtr m_container;
    };

}

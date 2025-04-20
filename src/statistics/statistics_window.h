#pragma once

#include "statistics.h"
#include "ui/gui_sdl.h"

#include <memory>
#include <string>

namespace Statistics
{

    class Window : public Ui::SDLWindow
    {
    public:
        using SPtr = std::shared_ptr<Window>;

        Window(uint32_t width, uint32_t height, const std::string &title);

        auto getStatisticsContainer() -> Container::SPtr;

        /// @brief Update window with latest frame data
        auto update() -> void;

    private:
        Container::SPtr m_container;
    };

}

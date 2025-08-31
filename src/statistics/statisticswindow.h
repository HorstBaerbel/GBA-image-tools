#pragma once

#include "statistics.h"
#include "ui/colorformat.h"
#include "ui/sdlwindow.h"

#include <memory>
#include <string>
#include <vector>

namespace Statistics
{

    class Window : public Ui::SDLWindow
    {
    public:
        using SPtr = std::shared_ptr<Window>;

        Window(uint32_t width, uint32_t height, const std::string &title);

        auto getStatisticsContainer() -> Container::SPtr;

        auto displayImage(const std::vector<uint8_t> &image, Ui::ColorFormat format, uint32_t width, uint32_t height, int32_t x = 0, int32_t y = 0) -> void;
        auto displayImage(const uint8_t *image, std::size_t size, Ui::ColorFormat format, uint32_t width, uint32_t height, int32_t x = 0, int32_t y = 0) -> void;

        /// @brief Update window with latest frame data
        auto update() -> void;

    private:
        auto userEvent(SDL_Event event) -> void override;

        struct DisplayImage
        {
            Ui::ColorFormat format = Ui::ColorFormat::Unknown;
            std::vector<uint8_t> image;
            uint32_t width = 0;
            uint32_t height = 0;
            int32_t x = 0;
            int32_t y = 0;
        };

        std::deque<std::variant<DisplayImage>> m_eventData;
        Container::SPtr m_container;
    };

}

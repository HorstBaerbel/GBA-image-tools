#include "statistics_window.h"

namespace Statistics
{

    Window::Window(uint32_t width, uint32_t height)
        : SDLWindow(width, height), m_container(std::make_shared<Container>())
    {
    }

    auto Window::getStatisticsContainer() -> Container::SPtr
    {
        return m_container;
    }

    auto Window::update() -> void
    {
        auto frames = m_container->getFrames();
        for (const auto &frame : frames)
        {
            auto images = frame->getImages();
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

}

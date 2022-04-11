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
        auto images = m_container->getImages();
        for (const auto &image : images)
        {
            const auto &data = image.second;
            switch (data.colorFormat)
            {
            case Image::ColorFormat::RGB888:
                displayImageRGB888(data.image, data.width, data.height);
                break;
            case Image::ColorFormat::RGB555:
                displayImageRGB555(data.image, data.width, data.height);
                break;
            }
        }
    }

}

#include "statistics.h"

namespace Statistics
{

    auto Container::addValue(const std::string &id, double v) -> void
    {
        m_values[id].push_back(v);
    }

    auto Container::addImage(const std::string &id, const std::vector<uint8_t> &image, Image::ColorFormat colorFormat, uint32_t width, uint32_t height) -> void
    {
        m_images[id] = {image, colorFormat, width, height};
    }

    auto Container::addImage(const std::string &id, std::vector<uint8_t> &&image, Image::ColorFormat colorFormat, uint32_t width, uint32_t height) -> void
    {
        m_images[id] = {std::move(image), colorFormat, width, height};
    }

    auto Container::getValues() const -> const std::map<std::string, std::vector<double>> &
    {
        return m_values;
    }

    auto Container::getImages() const -> const std::map<std::string, ImageData> &
    {
        return m_images;
    }

}

#include "statistics.h"

#include "exception.h"

namespace Statistics
{

    auto Frame::setValue(const std::string &id, double value, std::size_t index) -> void
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        auto vIt = m_values.find(id);
        if (vIt == m_values.end())
        {
            // create new value storage
            std::vector<double> values(index + 1, 0.0F);
            m_values[id] = values;
        }
        else if (index >= vIt->second.size())
        {
            // resize value storage
            auto &values = m_values[id];
            values.resize(index + 1, 0.0F);
        }
        // store value
        m_values[id].at(index) = value;
    }

    auto Frame::incValue(const std::string &id, double increase, std::size_t index) -> void
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        auto vIt = m_values.find(id);
        if (vIt == m_values.end())
        {
            // create new value storage
            std::vector<double> values(index + 1, 0.0F);
            m_values[id] = values;
        }
        else if (index >= vIt->second.size())
        {
            // resize value storage
            auto &values = m_values[id];
            values.resize(index + 1, 0.0F);
        }
        // increase value
        m_values[id].at(index) += increase;
    }

    auto Frame::getValue(const std::string &id, std::size_t index) const -> double
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        auto vIt = m_values.find(id);
        return (vIt == m_values.cend() || vIt->second.size() <= index) ? 0.0 : vIt->second.at(index);
    }

    auto Frame::getValues() const -> std::map<std::string, std::vector<double>>
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        return m_values;
    }

    auto Frame::setImage(const std::string &id, const std::vector<uint8_t> &image, Color::Format colorFormat, uint32_t width, uint32_t height) -> void
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_images[id] = {image, colorFormat, width, height};
    }

    auto Frame::setImage(const std::string &id, std::vector<uint8_t> &&image, Color::Format colorFormat, uint32_t width, uint32_t height) -> void
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_images[id] = {std::move(image), colorFormat, width, height};
    }

    auto Frame::getImages() const -> std::map<std::string, ImageData>
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        return m_images;
    }

    auto Container::addFrame() -> Frame::SPtr
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        auto newFrame = std::make_shared<Frame>();
        m_frames.push_back(newFrame);
        return newFrame;
    }

    auto Container::getFrames() const -> std::vector<Frame::SPtr>
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        return m_frames;
    }

    auto setValue(Frame::SPtr statistics, const std::string &id, double value, std::size_t index) -> void
    {
        if (statistics != nullptr)
        {
            statistics->setValue(id, value, index);
        }
    }
    auto incValue(Frame::SPtr statistics, const std::string &id, double increase, std::size_t index) -> void
    {
        if (statistics != nullptr)
        {
            statistics->incValue(id, increase, index);
        }
    }

    auto setImage(Frame::SPtr statistics, const std::string &id, const std::vector<uint8_t> &image, Color::Format colorFormat, uint32_t width, uint32_t height) -> void
    {
        if (statistics != nullptr)
        {
            statistics->setImage(id, image, colorFormat, width, height);
        }
    }

    auto setImage(Frame::SPtr statistics, const std::string &id, std::vector<uint8_t> &&image, Color::Format colorFormat, uint32_t width, uint32_t height) -> void
    {
        if (statistics != nullptr)
        {
            statistics->setImage(id, image, colorFormat, width, height);
        }
    }
}

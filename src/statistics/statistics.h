#pragma once

#include "image/imagestructs.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Statistics
{

    /// @brief One thread-safe frame of statistics data
    class Frame
    {
    public:
        using SPtr = std::shared_ptr<Frame>;

        struct ImageData
        {
            std::vector<uint8_t> image;
            Color::Format colorFormat = Color::Format::Unknown;
            uint32_t width = 0;
            uint32_t height = 0;
        };

        /// @brief Set frame statistics value. Create value if it does not exists and initialize to 0.0
        /// @param id Value identifier
        /// @param value Value to set
        /// @param index Value index
        auto setValue(const std::string &id, double value, std::size_t index = 0) -> void;

        /// @brief Increase a frame statistics value. Create value if it does not exists and initialize to increase
        /// @param id Value identifier
        /// @param increase Value to add to statistics value
        /// @param index Value index
        auto incValue(const std::string &id, double increase = 1, std::size_t index = 0) -> void;

        /// @brief Get frame statistics value
        /// @param id Value identifier
        /// @param index Value index
        /// @return Statistics value or 0.0 if id or index do not exist
        auto getValue(const std::string &id, std::size_t index = 0) const -> double;

        /// @brief Get frame all statistics values
        auto getValues() const -> std::map<std::string, std::vector<double>>;

        /// @brief Set frame statistics image
        /// @param id Image identifier
        /// @param image Image data
        /// @param colorFormat Image color format
        /// @param width Image width
        /// @param height Image height
        auto setImage(const std::string &id, const std::vector<uint8_t> &image, Color::Format colorFormat, uint32_t width, uint32_t height) -> void;

        /// @brief Set and move frame statistics image
        /// @param id Image identifier
        /// @param image Image data
        /// @param colorFormat Image color format
        /// @param width Image width
        /// @param height Image height
        auto setImage(const std::string &id, std::vector<uint8_t> &&image, Color::Format colorFormat, uint32_t width, uint32_t height) -> void;

        /// @brief Get all frame statistics images
        auto getImages() const -> std::map<std::string, ImageData>;

    private:
        std::map<std::string, std::vector<double>> m_values;
        std::map<std::string, ImageData> m_images;
        mutable std::mutex m_mutex;
    };

    /// @brief A thread-safe container holding multiple statistics frames
    class Container
    {
    public:
        using SPtr = std::shared_ptr<Container>;

        /// @brief Create new statistics frame
        /// @return New statistics frame
        auto addFrame() -> Frame::SPtr;

        /// @brief Return all added statistics frames
        auto getFrames() const -> std::vector<Frame::SPtr>;

    private:
        std::vector<Frame::SPtr> m_frames;
        mutable std::mutex m_mutex;
    };

    /// @brief Set frame statistics value if statistics != nullptr. Create value if it does not exists and initialize to 0.0
    /// @param statistics Frame statistics
    /// @param id Value identifier
    /// @param value Value to set
    /// @param index Value index
    auto setValue(Frame::SPtr statistics, const std::string &id, double value, std::size_t index = 0) -> void;

    /// @brief Increase a frame statistics value if statistics != nullptr. Create value if it does not exists and initialize to increase
    /// @param id Value identifier
    /// @param increase Value to add to statistics value
    /// @param index Value index
    auto incValue(Frame::SPtr statistics, const std::string &id, double increase = 1, std::size_t index = 0) -> void;

    /// @brief Set frame statistics image if statistics != nullptr
    /// @param id Image identifier
    /// @param image Image data
    /// @param colorFormat Image color format
    /// @param width Image width
    /// @param height Image height
    auto setImage(Frame::SPtr statistics, const std::string &id, const std::vector<uint8_t> &image, Color::Format colorFormat, uint32_t width, uint32_t height) -> void;

    /// @brief Set and move frame statistics image if statistics != nullptr
    /// @param id Image identifier
    /// @param image Image data
    /// @param colorFormat Image color format
    /// @param width Image width
    /// @param height Image height
    auto setImage(Frame::SPtr statistics, const std::string &id, std::vector<uint8_t> &&image, Color::Format colorFormat, uint32_t width, uint32_t height) -> void;
}

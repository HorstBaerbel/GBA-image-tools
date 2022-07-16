#pragma once

#include "processing/imagestructs.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Statistics
{

    class Container
    {
    public:
        using SPtr = std::shared_ptr<Container>;

        struct ImageData
        {
            std::vector<uint8_t> image;
            Image::ColorFormat colorFormat = Image::ColorFormat::Unknown;
            uint32_t width = 0;
            uint32_t height = 0;
        };

        auto addValue(const std::string &id, double v) -> void;

        auto addImage(const std::string &id, const std::vector<uint8_t> &image, Image::ColorFormat colorFormat, uint32_t width, uint32_t height) -> void;

        auto addImage(const std::string &id, std::vector<uint8_t> &&image, Image::ColorFormat colorFormat, uint32_t width, uint32_t height) -> void;

        auto getValues() const -> const std::map<std::string, std::vector<double>> &;
        auto getImages() const -> const std::map<std::string, ImageData> &;

    private:
        std::map<std::string, std::vector<double>> m_values;
        std::map<std::string, ImageData> m_images;
    };

}

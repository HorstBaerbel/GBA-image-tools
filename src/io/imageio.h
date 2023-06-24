#pragma once

#include "exception.h"
#include "processing/imagestructs.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace IO
{

    class File
    {
    public:
        /// @brief Read image from disk and return as linear XRGB8888 color data
        /// @note Does NOT set the index or file name part of Data
        static auto readImage(const std::string &filePath) -> Image::Data;

        /// @brief Write image data to image file
        static auto writeImage(const std::string &folder, const Image::Data &image) -> void;

        /// @brief Write image data to image file
        static auto writeImages(const std::string &folder, const std::vector<Image::Data> &images) -> void;
    };

}

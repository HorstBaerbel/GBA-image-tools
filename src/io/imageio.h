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
        /// @brief Write image data to image file
        static auto writeImage(const std::string &folder, const Image::Data &image) -> void;

        /// @brief Write image data to image file
        static auto writeImages(const std::string &folder, const std::vector<Image::Data> &images) -> void;
    };

}

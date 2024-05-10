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
        /// @param filePath Path to image file
        static auto readImage(const std::string &filePath) -> Image::Data;

        /// @brief Write image data to image file
        /// @param image Image data to write. If image.fileName is filled it can be used as the file name
        /// @param folder Image output folder
        /// @param fileName Optional. If passed used as image file name, if empty image.fileName is used
        static auto writeImage(const Image::Data &image, const std::string &folder, const std::string &fileName = "") -> void;

        /// @brief Write image data to image file
        /// @param images Images data to write. If image.fileName is filled it can be used as the file name
        /// @param folder Images output folder
        static auto writeImages(const std::vector<Image::Data> &images, const std::string &folder) -> void;

        /// @brief Write raw image data to file
        /// @param image Image data to write. If image.fileName is filled it can be used as the file name
        /// @param folder Image output folder
        /// @param fileName Optional. If passed used as image file name, if empty image.fileName is used
        static auto writeRawImage(const Image::Data &image, const std::string &folder, const std::string &fileName = "") -> void;
    };

}

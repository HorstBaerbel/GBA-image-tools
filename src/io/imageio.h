#pragma once

#include "exception.h"
#include "image/imagestructs.h"

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
        static auto readImage(const std::string &filePath) -> Image::Frame;

        /// @brief Write image data to PNG image file
        /// @param image Image data to write. If image.fileName is filled it can be used as the file name
        /// @param folder Image output folder. If empty, only fileName will be used, if filled
        /// @param fileName Optional. If passed used as image file name, if empty image.fileName is used
        /// @note Will create necessary directories if not found
        static auto writeImage(const Image::Frame &image, const std::string &folder, const std::string &fileName = "") -> void;

        /// @brief Write image data to PNG image files
        /// @param images Images data to write. image.fileName must be filled
        /// @param folder Images output folder
        /// @note Will create necessary directories if not found
        static auto writeImages(const std::vector<Image::Frame> &images, const std::string &folder) -> void;

        /// @brief Write raw image data to file
        /// @param image Image data to write. If image.fileName is filled it can be used as the file name
        /// @param folder Image output folder
        /// @param fileName Optional. If passed used as image file name, if empty image.fileName is used
        /// @note Will create necessary directories if not found
        static auto writeRawImage(const Image::Frame &image, const std::string &folder, const std::string &fileName = "") -> void;
    };

}

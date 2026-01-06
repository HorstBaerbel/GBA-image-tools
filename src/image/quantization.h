#pragma once

#pragma once

#include "imagedata.h"

namespace Image
{

    namespace Quantization
    {

        /// @brief Quantize pixel data using thresholding
        /// @param[in] data Input image data
        /// @param[in] threshold Greyscale quatization threshold in range [0,1]
        /// @return Returns pixel data quantized and converted to Color::Format::Paletted8
        auto quantizeThreshold(const ImageData &data, float threshold) -> ImageData;

        /// @brief Quantize pixel data by choosing closest colors from given palette using cluster-fit
        /// @param[in] data Input image data
        /// @param[in] colorMapping Mapping of target color -> source colors
        /// @return Returns pixel data quantized and converted to Color::Format::Paletted8
        auto quantizeClosest(const ImageData &data, const std::map<Color::XRGB8888, std::vector<Color::XRGB8888>> &colorMapping) -> ImageData;

        /// @brief Quantize pixel data using Atkison error-diffusion dither and choosing colors from given palette
        /// @param[in] data Input image data
        /// @param[in] width Image width
        /// @param[in] height Image height
        /// @param[in] colorMapping Mapping of target color -> source colors
        /// @return Returns pixel data quantized and converted to Color::Format::Paletted8
        auto atkinsonDither(const ImageData &data, uint32_t width, uint32_t height, const std::map<Color::XRGB8888, std::vector<Color::XRGB8888>> &colorMapping) -> ImageData;
    }

}

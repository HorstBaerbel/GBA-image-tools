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
        /// @param[in] nrOfColors Number of colors to reduce image to
        /// @return Returns pixel data quantized and converted to Color::Format::Paletted8
        auto quantizeClosest(const ImageData &data, uint32_t nrOfColors, const std::vector<Color::XRGB8888> &colorMap) -> ImageData;

        /// @brief Quantize pixel data using Atkison error-diffusion dither and choosing colors from given palette
        /// @param[in] data Input image data
        /// @param[in] nrOfColors Number of colors to reduce image to
        /// @return Returns pixel data quantized and converted to Color::Format::Paletted8
        auto atkinsonDither(const ImageData &data, uint32_t nrOfColors, const std::vector<Color::XRGB8888> &colorMap) -> ImageData;

    }

}

#pragma once

#include <string>

namespace Image
{

    namespace Quantization
    {

        /// @brief Quantization method
        enum class Method
        {
            None = 0,
            ClosestColor = 1,  // Choose closest color in target color space and colormap
            AtkinsonDither = 2 // Dither image using the Atkinson dithering algorithm
        };

        /// @brief Return quantization method as string
        auto toString(Method method) -> std::string;

        /// @brief Return quantization method for string
        auto toMethod(const std::string &method) -> Method;

    }

}

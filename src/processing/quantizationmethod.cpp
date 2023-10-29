#include "quantizationmethod.h"

#include "exception.h"

namespace Image
{

    auto Quantization::toString(Method method) -> std::string
    {
        switch (method)
        {
        case Method::ClosestColor:
            return "Closest color";
        case Method::AtkinsonDither:
            return "Atkinson dither";
        default:
            THROW(std::runtime_error, "Bad quantization method");
        }
    }

}

// Generates a map of all 32768 displayable colors in RGB555 color space and as colors grouped by hue
// You'll need libmagick++-dev installed!

#include "color/colorhelpers.h"

#include <Magick++.h>

// Redefine QuantumRange here, to avoid an issue with ImageMagick headers
#if (MAGICKCORE_QUANTUM_DEPTH == 8)
#define QuantumRange (255.0)
#elif (MAGICKCORE_QUANTUM_DEPTH == 16)
#define QuantumRange (65535.0)
#elif (MAGICKCORE_QUANTUM_DEPTH == 32)
#define QuantumRange (4294967295.0)
#elif (MAGICKCORE_QUANTUM_DEPTH == 64)
#define QuantumRange (18446744073709551615.0)
#else
#error "Could not define QuantumRange, check the ImageMagick header"
#endif

bool isLessThan(const Magick::ColorRGB &a, const Magick::ColorRGB &b)
{
    Magick::ColorHSL hsla = a;
    Magick::ColorHSL hslb = b;
    return (hsla.hue() < hslb.hue()) ||
           (hsla.hue() == hslb.hue() && (hsla.lightness() * hsla.saturation()) < (hslb.lightness() * hslb.saturation()));
}

int main(int argc, char *argv[])
{
    Magick::InitializeMagick(*argv);
    buildColorMapRGB555().write("colormap555.png");
    std::vector<Magick::ColorRGB> colors;
    for (uint8_t r = 0; r < 32; ++r)
    {
        for (uint8_t g = 0; g < 32; ++g)
        {
            for (uint8_t b = 0; b < 32; ++b)
            {
                colors.push_back(Magick::ColorRGB(r / 31.0, g / 31.0, b / 31.0));
            }
        }
    }
    std::sort(colors.begin(), colors.end(), isLessThan);
    Magick::Image image2(Magick::Geometry(256, 128), "black");
    image2.type(Magick::ImageType::TrueColorType);
    image2.modifyImage();
    auto pixels2 = image2.getPixels(0, 0, image2.columns(), image2.rows());
    for (uint32_t i = 0; i < image2.columns() * image2.rows(); ++i)
    {
        *pixels2++ = QuantumRange * static_cast<float>(colors[i].red());
        *pixels2++ = QuantumRange * static_cast<float>(colors[i].green());
        *pixels2++ = QuantumRange * static_cast<float>(colors[i].blue());
    }

    image2.syncPixels();
    image2.write("colormap555_hsl.png");
    return 0;
}
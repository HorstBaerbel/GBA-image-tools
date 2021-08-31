// Generates a map of all 32768 displayable colors in RGB555 color space and as colors grouped by hue
// You'll need libmagick++-dev installed!

#include "colorhelpers.h"

#include <Magick++.h>

using namespace Magick;

bool isLessThan(const Color &a, const Color &b)
{
    ColorHSL hsla = a;
    ColorHSL hslb = b;
    return (hsla.hue() < hslb.hue()) ||
           (hsla.hue() == hslb.hue() && (hsla.luminosity() * hsla.saturation()) < (hslb.luminosity() * hslb.saturation()));
}

int main(int argc, char *argv[])
{
    InitializeMagick(*argv);
    buildColorMapRGB555().write("colormap555.png");
    std::vector<Color> colors;
    for (uint8_t r = 0; r < 32; ++r)
    {
        for (uint8_t g = 0; g < 32; ++g)
        {
            for (uint8_t b = 0; b < 32; ++b)
            {
                colors.push_back(ColorRGB(r / 31.0, g / 31.0, b / 31.0));
            }
        }
    }
    std::sort(colors.begin(), colors.end(), isLessThan);
    Image image2(Geometry(256, 128), "black");
    image2.type(TrueColorType);
    image2.modifyImage();
    auto pixels2 = image2.getPixels(0, 0, image2.columns(), image2.rows());
    for (uint32_t i = 0; i < image2.columns() * image2.rows(); ++i)
    {
        *pixels2++ = colors[i];
    }
    image2.syncPixels();
    image2.write("colormap555_hsl.png");
    return 0;
}
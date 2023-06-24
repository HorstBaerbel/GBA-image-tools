// Generates a map of all 32768 displayable colors in RGB555 color space and as colors grouped by hue
// You'll need libmagick++-dev installed!

#include "color/colorhelpers.h"
#include "color/lchf.h"
#include "io/imageio.h"

bool isLessThan(const Color::XRGB8888 &a, const Color::XRGB8888 &b)
{
    auto lcha = Color::convertTo<Color::LChf>(a);
    auto lchb = Color::convertTo<Color::LChf>(b);
    return (lcha.H() < lchb.H()) ||
           (lcha.H() < lchb.H() && (lcha.L() * lcha.C()) < (lchb.L() * lchb.C()));
}

int main(int argc, char *argv[])
{
    // Generate colors
    std::vector<Color::XRGB8888> colors;
    for (uint8_t r = 0; r < 32; ++r)
    {
        for (uint8_t g = 0; g < 32; ++g)
        {
            for (uint8_t b = 0; b < 32; ++b)
            {
                colors.push_back(Color::XRGB8888(r / 31.0, g / 31.0, b / 31.0));
            }
        }
    }
    Image::ImageData pixelsRGB(colors);
    Image::Data data;
    data.fileName = "colormap555.png";
    data.size = {256, 128};
    data.imageData = pixelsRGB;
    IO::File::writeImage(".", data);
    // Sort colors
    std::sort(colors.begin(), colors.end(), isLessThan);
    Image::ImageData pixelsLCH(colors);
    data.fileName = "colormap555_lchf.png";
    data.size = {256, 128};
    data.imageData = pixelsLCH;
    IO::File::writeImage(".", data);
    return 0;
}
// Generates a map of all 32768 displayable colors in RGB555 color space and as colors grouped by hue
// You'll need libmagick++-dev installed!

#include "color/colorhelpers.h"
#include "color/lchf.h"
#include "io/imageio.h"

int main(int argc, char *argv[])
{
    // Generate RGB555 colors
    std::vector<Color::XRGB8888> colors555;
    for (uint8_t r = 0; r < 32; ++r)
    {
        for (uint8_t g = 0; g < 32; ++g)
        {
            for (uint8_t b = 0; b < 32; ++b)
            {
                colors555.push_back(Color::XRGB8888((255.0F * r) / 31.0F, (255.0F * g) / 31.0F, (255.0F * b) / 31.0F));
            }
        }
    }
    Image::ImageData pixels555(colors555);
    Image::Data data555;
    data555.image.size = {256, 128};
    data555.image.data = pixels555;
    IO::File::writeImage(data555, ".", "colormap555.png");
    // Generate RGB565 colors
    std::vector<Color::XRGB8888> colors565;
    for (uint8_t r = 0; r < 32; ++r)
    {
        for (uint8_t g = 0; g < 64; ++g)
        {
            for (uint8_t b = 0; b < 32; ++b)
            {
                colors565.push_back(Color::XRGB8888((255.0F * r) / 31.0F, (255.0F * g) / 63.0F, (255.0F * b) / 31.0F));
            }
        }
    }
    Image::ImageData pixels565(colors565);
    Image::Data data565;
    data565.image.size = {256, 256};
    data565.image.data = pixels565;
    IO::File::writeImage(data565, ".", "colormap565.png");
    return 0;
}
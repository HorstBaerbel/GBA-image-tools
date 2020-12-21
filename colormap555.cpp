// Generates a map of all 32768 displayable colors in RGB555 color space
// You'll need the libmagick++-dev installed!

#include <Magick++.h>

using namespace Magick;

int main(int argc, char *argv[])
{
    InitializeMagick(*argv);
    std::vector<uint8_t> pixels;
    for (uint8_t r = 0; r < 32; ++r)
    {
        for (uint8_t g = 0; g < 32; ++g)
        {
            for (uint8_t b = 0; b < 32; ++b)
            {
                pixels.push_back((255 * r) / 31);
                pixels.push_back((255 * g) / 31);
                pixels.push_back((255 * b) / 31);
            }
        }
    }
    Image image(256, 128, "RGB", CharPixel, pixels.data());
    image.type(TrueColorType);
    image.write("colormap555.png");
    return 0;
}
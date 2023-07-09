// Generates a Gimp palette file of all 32768 displayable colors in RGB555 color space
// The GBA.gpl file generated can be imported into Gimp or put into the
// /usr/share/gimp/2.0/palettes folder on Linux.
// Don't ask me where it resides in Windows...

#include <vector>
#include <fstream>
#include <iomanip>
#include <cstdint>

int main(int argc, char *argv[])
{
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
    std::ofstream of;
    of.open("GBA.gpl", std::fstream::trunc);
    of << "GIMP Palette" << std::endl;
    of << "Name: Game Boy Advance RGB555" << std::endl;
    of << "Columns: 256" << std::endl;
    for (uint32_t i = 0; i < pixels.size(); i += 3)
    {
        of << std::right << std::dec << std::setw(3) << std::setfill(' ') << (int32_t)pixels[i] << " ";
        of << std::right << std::dec << std::setw(3) << std::setfill(' ') << (int32_t)pixels[i + 1] << " ";
        of << std::right << std::dec << std::setw(3) << std::setfill(' ') << (int32_t)pixels[i + 2];
        of << "\t#";
        of << std::right << std::hex << std::setw(2) << std::setfill('0') << (int32_t)pixels[i];
        of << std::right << std::hex << std::setw(2) << std::setfill('0') << (int32_t)pixels[i + 1];
        of << std::right << std::hex << std::setw(2) << std::setfill('0') << (int32_t)pixels[i + 2];
        of << std::endl;
    }
    of.close();
    return 0;
}
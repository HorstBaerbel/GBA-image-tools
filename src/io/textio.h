#pragma once

#include "exception.h"

#include <iomanip>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace IO
{

    class Text
    {
    private:
        /// @brief Write values as a comma-separated array of hex numbers.
        template <typename T>
        static void writeValues(std::ofstream &outFile, const std::vector<T> &data, bool asHex = false)
        {
            auto flags = outFile.flags();
            size_t loopCount = 0;
            for (auto current : data)
            {
                if (asHex)
                {
                    outFile << "0x" << std::hex << std::noshowbase << std::setw(sizeof(T) * 2) << std::setfill('0') << current;
                }
                else
                {
                    outFile << std::dec << current;
                }
                if (loopCount < data.size() - 1)
                {
                    outFile << ", ";
                }
                if (++loopCount % 10 == 0)
                {
                    outFile << std::endl;
                }
            }
            outFile.flags(flags);
        }

    public:
        /// @brief Write image information to a .h file.
        static void writeImageInfoToH(std::ofstream &hFile, const std::string &varName, const std::vector<uint32_t> &data, const std::vector<uint32_t> &mapData, uint32_t width, uint32_t height, uint32_t bytesPerImage, uint32_t nrOfImages = 1, bool asTiles = false);

        /// @brief Write additional palette information to a .h file. Use after write writeImageInfoToH.
        template <typename T>
        static void writePaletteInfoToHeader(std::ofstream &hFile, const std::string &varName, const std::vector<T> &data, uint32_t nrOfColors, bool singleColorMap = true, bool asTiles = false)
        {
            hFile << "#define " << varName << "_PALETTE_LENGTH " << nrOfColors << " // # of palette entries per palette" << std::endl;
            hFile << "#define " << varName << "_PALETTE_SIZE " << data.size() << " // size of palette data" << std::endl;
            if (!singleColorMap)
            {
                hFile << "extern const uint32_t " << varName << "_PALETTE_START[" << varName << (asTiles ? "_NR_OF_TILES]; // index where a palette for a sprite/tile starts (in 2 byte units)" : "_NR_OF_IMAGES]; // index where a palette for an image starts (in 2 byte units)") << std::endl;
            }
            hFile << "extern const uint16_t " << varName << "_PALETTE[" << varName << "_PALETTE_SIZE];" << std::endl;
        }

        /// @brief Write image data to a .c file.
        static void writeImageDataToC(std::ofstream &cFile, const std::string &varName, const std::string &hFileBaseName, const std::vector<uint32_t> &data, const std::vector<uint32_t> &startIndices = std::vector<uint32_t>(), const std::vector<uint32_t> &mapData = std::vector<uint32_t>(), bool asTiles = false);

        /// @brief Write palette data to a .c file. Use after write writeImageDataToC.
        template <typename T>
        static void writePaletteDataToC(std::ofstream &cFile, const std::string &varName, const std::vector<T> &data, const std::vector<uint32_t> &startIndices = std::vector<uint32_t>(), bool asTiles = false)
        {
            // write palette start indices if more than one palette
            if (startIndices.size() > 1)
            {
                cFile << "const _Alignas(4) uint32_t " << varName << "_PALETTE_START[" << varName << (asTiles ? "_NR_OF_TILES] = { " : "_NR_OF_IMAGES] = { ") << std::endl;
                writeValues(cFile, startIndices);
                cFile << "};" << std::endl
                      << std::endl;
            }
            // write palette data
            cFile << "const _Alignas(4) ";
            static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4);
            if constexpr (sizeof(T) == 1)
            {
                cFile << "uint8_t";
            }
            else if constexpr (sizeof(T) == 2)
            {
                cFile << "uint16_t";
            }
            else if constexpr (sizeof(T) == 3)
            {
                cFile << "uint32_t";
            }
            cFile << " " << varName << "_PALETTE[" << varName << "_PALETTE_SIZE] = { " << std::endl;
            writeValues(cFile, data, true);
            cFile << "};" << std::endl
                  << std::endl;
        }
    };

}

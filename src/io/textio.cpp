#include "textio.h"

#include <iomanip>
#include <iostream>
#include <filesystem>

/// @brief Write values as a comma-separated array of hex numbers.
template <typename T>
void writeValues(std::ofstream &outFile, const std::vector<T> &data, bool asHex = false)
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

void writeImageInfoToH(std::ofstream &hFile, const std::string &varName, const std::vector<uint32_t> &data, const std::vector<uint32_t> &mapData, uint32_t width, uint32_t height, uint32_t bytesPerImage, uint32_t nrOfImages, bool asTiles)
{
    hFile << "#pragma once" << std::endl;
    hFile << "#include <stdint.h>" << std::endl
          << std::endl;
    if (asTiles)
    {
        hFile << "#define " << varName << "_WIDTH " << width << " // width of sprites/tiles in pixels" << std::endl;
        hFile << "#define " << varName << "_HEIGHT " << height << " // height of sprites/tiles in pixels" << std::endl;
        hFile << "#define " << varName << "_BYTES_PER_TILE " << bytesPerImage << " // bytes for one complete sprite/tile" << std::endl;
        hFile << "#define " << varName << "_DATA_SIZE " << data.size() << " // size of sprite/tile data in 4 byte units" << std::endl;
    }
    else
    {
        hFile << "#define " << varName << "_WIDTH " << width << " // width of image in pixels" << std::endl;
        hFile << "#define " << varName << "_HEIGHT " << height << " // height of image in pixels" << std::endl;
        hFile << "#define " << varName << "_BYTES_PER_IMAGE " << bytesPerImage << " // bytes for one complete image" << std::endl;
        hFile << "#define " << varName << "_DATA_SIZE " << data.size() << " // size of image data in 4 byte units" << std::endl;
    }
    if (!mapData.empty())
    {
        hFile << "#define " << varName << "_MAPDATA_SIZE " << mapData.size() << " // size of screen map data in 4 byte units" << std::endl;
    }
    if (nrOfImages > 1)
    {
        if (asTiles)
        {
            hFile << "#define " << varName << "_NR_OF_TILES " << nrOfImages << " // # of sprites/tiles in data" << std::endl;
        }
        else
        {
            hFile << "#define " << varName << "_NR_OF_IMAGES " << nrOfImages << " // # of images in data" << std::endl;
            hFile << "extern const uint32_t " << varName << "_DATA_START[" << varName << "_NR_OF_IMAGES]; // indices where data for an image starts (in 4 byte units)" << std::endl;
        }
    }
    hFile << "extern const uint32_t " << varName << "_DATA[" << varName << "_DATA_SIZE];" << std::endl;
    if (!mapData.empty())
    {
        hFile << "extern const uint32_t " << varName << "_MAPDATA[" << varName << "_MAPDATA_SIZE];" << std::endl;
    }
}

void writePaletteInfoToHeader(std::ofstream &hFile, const std::string &varName, const std::vector<uint16_t> &data, uint32_t nrOfColors, bool singleColorMap, bool asTiles)
{
    hFile << "#define " << varName << "_PALETTE_LENGTH " << nrOfColors << " // # of palette entries per palette" << std::endl;
    hFile << "#define " << varName << "_PALETTE_SIZE " << data.size() << " // size of palette data in 2 byte units" << std::endl;
    if (!singleColorMap)
    {
        hFile << "extern const uint32_t " << varName << "_PALETTE_START[" << varName << (asTiles ? "_NR_OF_TILES]; // index where a palette for a sprite/tile starts (in 2 byte units)" : "_NR_OF_IMAGES]; // index where a palette for an image starts (in 2 byte units)") << std::endl;
    }
    hFile << "extern const uint16_t " << varName << "_PALETTE[" << varName << "_PALETTE_SIZE];" << std::endl;
}

void writeImageDataToC(std::ofstream &cFile, const std::string &varName, const std::string &hFileBaseName, const std::vector<uint32_t> &data, const std::vector<uint32_t> &dataStartIndices, const std::vector<uint32_t> &mapData, bool asTiles)
{
    cFile << "#include \"" << hFileBaseName << ".h\"" << std::endl
          << std::endl;
    // write map data if passed
    if (!mapData.empty())
    {
        cFile << "const _Alignas(4) uint32_t " << varName << "_MAPDATA[" << varName << "_MAPDATA_SIZE] = { " << std::endl;
        writeValues(cFile, mapData, true);
        cFile << "};" << std::endl
              << std::endl;
    }
    // write data start indices if passed
    if (dataStartIndices.size() > 1)
    {
        cFile << "const _Alignas(4) uint32_t " << varName << "_DATA_START[" << varName << (asTiles ? "_NR_OF_TILES] = { " : "_NR_OF_IMAGES] = { ") << std::endl;
        writeValues(cFile, dataStartIndices);
        cFile << "};" << std::endl
              << std::endl;
    }
    // write image data
    cFile << "const _Alignas(4) uint32_t " << varName << "_DATA[" << varName << "_DATA_SIZE] = { " << std::endl;
    writeValues(cFile, data, true);
    cFile << "};" << std::endl
          << std::endl;
}

void writePaletteDataToC(std::ofstream &cFile, const std::string &varName, const std::vector<uint16_t> &data, const std::vector<uint32_t> &startIndices, bool asTiles)
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
    cFile << "const _Alignas(4) uint16_t " << varName << "_PALETTE[" << varName << "_PALETTE_SIZE] = { " << std::endl;
    writeValues(cFile, data, true);
    cFile << "};" << std::endl
          << std::endl;
}

#include "textio.h"

#include <iostream>
#include <filesystem>

namespace IO
{

    auto Text::writeImageInfoToH(std::ofstream &hFile, const std::string &varName, const std::vector<uint32_t> &data, uint32_t width, uint32_t height, uint32_t bytesPerImage, uint32_t nrOfImages, bool asTiles) -> void
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
    }

    auto Text::writeMapInfoToH(std::ofstream &hFile, const std::string &varName, const std::vector<uint32_t> &mapData) -> void
    {
        if (!mapData.empty())
        {
            hFile << "#define " << varName << "_MAPDATA_SIZE " << mapData.size() << " // size of screen map data in 4 byte units" << std::endl;
            hFile << "extern const uint32_t " << varName << "_MAPDATA[" << varName << "_MAPDATA_SIZE];" << std::endl;
        }
    }

    auto Text::writeCompressionInfoToH(std::ofstream &hFile, const std::string &varName, uint32_t maxMemoryNeeded) -> void
    {
        hFile << "#define " << varName << "_DECOMPRESSION_BUFFER_SIZE " << maxMemoryNeeded << " // max. decompression buffer size needed for everything EXCEPT the last step" << std::endl;
    }

    auto Text::writeImageDataToC(std::ofstream &cFile, const std::string &varName, const std::string &hFileBaseName, const std::vector<uint32_t> &data, const std::vector<uint32_t> &dataStartIndices, bool asTiles) -> void
    {
        cFile << "#include \"" << hFileBaseName << ".h\"" << std::endl
              << std::endl;
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

    auto Text::writeMapDataToC(std::ofstream &cFile, const std::string &varName, const std::vector<uint32_t> &mapData) -> void
    {
        if (!mapData.empty())
        {
            cFile << "const _Alignas(4) uint32_t " << varName << "_MAPDATA[" << varName << "_MAPDATA_SIZE] = { " << std::endl;
            writeValues(cFile, mapData, true);
            cFile << "};" << std::endl
                  << std::endl;
        }
    }
}

#include "binio.h"

#include "exception.h"

#include <iostream>
#include <fstream>

namespace IO
{
    auto writeData(const std::string &fileName, const char *data, std::size_t size) -> void
    {
        std::ofstream binFile(fileName, std::ios::out | std::ios::binary);
        if (binFile.is_open())
        {
            std::cout << "Writing output file " << fileName << std::endl;
            try
            {
                binFile.write(data, size);
                REQUIRE(!binFile.bad(), std::runtime_error, "Failed to write data to output file");
            }
            catch (const std::runtime_error &e)
            {
                binFile.close();
                THROW(std::runtime_error, "Failed to write data to output file: " << e.what());
            }
        }
        else
        {
            binFile.close();
            THROW(std::runtime_error, "Failed to open " << fileName << " for writing");
        }
    }

    auto Bin::writeData(const std::string &fileName, const std::vector<uint8_t> &data) -> void
    {
        IO::writeData(fileName, reinterpret_cast<const char *>(data.data()), data.size());
    }

    auto Bin::writeData(const std::string &fileName, const std::vector<uint32_t> &data) -> void
    {
        IO::writeData(fileName, reinterpret_cast<const char *>(data.data()), data.size() * 4);
    }
}

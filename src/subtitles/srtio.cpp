#include "srtio.h"

#include "exception.h"

#include <cstdint>
#include <fstream>

namespace IO
{
    auto SRT::readSRT(const std::string &filePath) -> std::vector<Subtitles::Frame>
    {
        REQUIRE(!filePath.empty(), std::runtime_error, "filePath must contain a file name");
        // open input file
        auto fileSrt = std::ifstream(filePath, std::ios::in);
        REQUIRE(fileSrt.is_open() && !fileSrt.fail(), std::runtime_error, "Failed to open " << filePath << " for reading");
        // read everything into lines buffer
        std::vector<std::string> lines;
        while (!fileSrt.eof())
        {
            std::string line = fileSrt.
        }
        return {};
    }
}

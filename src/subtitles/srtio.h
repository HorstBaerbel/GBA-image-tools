#pragma once

#include "subtitlesstructs.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace IO
{

    class SRT
    {
    public:
        /// @brief Read SRT file
        /// @param filePath Path to SRT file
        /// @return Returns all subtitles in file
        /// @throw Throws if file can not be found or opened for reading or if subtitle format is wrong
        static auto readSRT(const std::string &filePath) -> std::vector<Subtitles::Frame>;
    };

}

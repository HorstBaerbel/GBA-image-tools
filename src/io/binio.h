#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace IO
{

    class Bin
    {
    public:
        /// @brief Write image information to a binary file
        static auto writeData(const std::string &fileName, const std::vector<uint8_t> &data) -> void;

        /// @brief Write image information to a binary file
        static auto writeData(const std::string &fileName, const std::vector<uint32_t> &data) -> void;
    };
}

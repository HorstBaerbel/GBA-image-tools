#pragma once

#include "exception.h"

#include <cstdint>
#include <fstream>
#include <vector>
#include <map>

namespace IO
{

    /// @brief Writes statistics to CSV files
    class StatisticsWriter
    {
    public:
        /// @brief Constructor
        StatisticsWriter() = default;

        /// @brief Destruktor. Calls close()
        virtual ~StatisticsWriter();

        /// @brief Open csv files for writing
        /// @param fileBasePath Base path to output file without extension, e.g. "results/output".
        /// An type id and extension will be added automatically, e.g. "_<TYPE>.csv"
        /// @param types Data type tags supported, e.g. "{audio, video, whatever}"
        /// @throws std::runtime_error when failing to open the file(s) for writing
        /// @note Will overwrite the file(s)
        auto open(const std::string &fileBasePath, const std::vector<std::string> &types) -> void;

        /// @brief Write statistics on frame data to a CSV file
        /// @param type Data type tag, e.g. "audio"
        /// @param frame Binary frame data
        /// @param compressionRatio Optional. Compression ratio of data in [0,1]
        /// @throws std::runtime_error when the type is not one passed in open()
        auto writeFrame(const std::string &type, const std::vector<uint8_t> &data, const float compressionRatio = 0.0F) -> void;

        /// @brief Close writer opened with open()
        auto close() -> void;

    private:
        std::map<std::string, std::ofstream> m_oss;
        std::map<std::string, uint32_t> m_frameIndex;
    };

}

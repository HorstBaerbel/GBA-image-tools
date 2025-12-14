#include "statisticswriter.h"

#include "exception.h"
#include "math/histogram.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>

namespace IO
{

    auto StatisticsWriter::open(const std::string &fileBasePath, const std::vector<std::string> &types) -> void
    {
        REQUIRE(!fileBasePath.empty(), std::runtime_error, "fileBasePath must contain a file name");
        REQUIRE(!types.empty(), std::runtime_error, "types must contain type tags");
        // open input file(s)
        for (const auto &t : types)
        {
            // build file name and open file
            auto filePath = fileBasePath + "_stats_" + t + ".csv";
            auto csvFile = std::ofstream(filePath, std::ios::out | std::ios::trunc);
            REQUIRE(csvFile.is_open() && !csvFile.fail(), std::runtime_error, "Failed to open " << filePath << " for writing");
            std::cout << "Writing statistics to " << filePath << std::endl;
            // write header
            csvFile << "frame,";
            for (std::size_t i = 0; i < 256; ++i)
            {
                csvFile << "f" << std::to_string(i) << ",";
            }
            csvFile << "alphabetsize,";
            csvFile << "entropy,";
            csvFile << "ratio" << std::endl;
            REQUIRE(!csvFile.fail(), std::runtime_error, "Writing headers to " << filePath << " failed");
            m_oss[t] = std::move(csvFile);
            m_frameIndex[t] = 0;
        }
    }

    auto StatisticsWriter::writeFrame(const std::string &type, const std::vector<uint8_t> &data, const float compressionRatio) -> void
    {
        REQUIRE(!type.empty(), std::runtime_error, "Must pass a type tag");
        REQUIRE(!data.empty(), std::runtime_error, "Data can not be empty");
        auto osIt = m_oss.find(type);
        REQUIRE(osIt != m_oss.end(), std::runtime_error, "Unknown type tag " << type);
        // calculate histogram
        auto histogram = Histogram::normalizeHistogram(Histogram::buildHistogramKeepEmpty(data));
        // std::cout << "Histogram size: " << histogram.size() << std::endl;
        // calculate normalized entropy of data
        uint32_t alphabetSize = 0;
        float entropy = 0.0F;
        for (const auto &entry : histogram)
        {
            if (entry.second > 0.0F)
            {
                alphabetSize++;
                entropy -= entry.second * std::log2(entry.second);
            }
        }
        // std::cout << "Entropy (bits): " << entropy << std::endl;
        // write data to file
        osIt->second << std::to_string(m_frameIndex[type]++) << ",";
        for (const auto &entry : histogram)
        {
            osIt->second << std::to_string(entry.second) << ",";
        }
        osIt->second << std::to_string(alphabetSize) << ",";
        osIt->second << std::to_string(entropy) << ",";
        osIt->second << std::to_string(compressionRatio) << std::endl;
    }

    auto StatisticsWriter::close() -> void
    {
        for (auto &os : m_oss)
        {
            os.second.close();
        }
        m_oss.clear();
    }

    StatisticsWriter::~StatisticsWriter()
    {
        close();
    }
}

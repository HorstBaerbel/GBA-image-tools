#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>

namespace IO
{

    class CSV
    {
    public:
        /// @brief Write CSV files to stream
        /// @tparam T Array, vector or map of value type
        /// @tparam ACCESS_FUNC Function to access then nth field from a T, think "values[i][n]". N is in this case [0,valueNames.size()]
        /// @param csvFile CSV file. Must be open and writable
        /// @param names Names of CSV values. Number determines how many indexes are read from each entry in values
        /// @param values Values to add to file. Accessed through accessFunc
        template <typename T, typename ACCESS_FUNC>
        static void writeCSV(std::ofstream &csvFile, const std::vector<std::string> &names, const T &values, ACCESS_FUNC accessFunc)
        {
            // write header
            for (std::size_t i = 0; i < names.size(); ++i)
            {
                csvFile << names[i] << (i < names.size() - 1 ? "," : "");
            }
            csvFile << std::endl;
            // write values
            for (const auto &value : values)
            {
                for (std::size_t i = 0; i < names.size(); ++i)
                {
                    csvFile << accessFunc(value, i) << (i < names.size() - 1 ? "," : "");
                }
                csvFile << std::endl;
            }
        }
    };

}

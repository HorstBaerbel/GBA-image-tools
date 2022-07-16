#include "lzss.h"

#include "exception.h"

#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif

#include <fstream>
#include <filesystem>

namespace Compression
{

    /// @brief Get environment variable from system.
    std::string getEnv(const std::string &var)
    {
        const char *value = std::getenv(var.c_str());
        return value != nullptr ? value : "";
    }

    static std::string GbaLzssPath;

    std::string findGbalzss()
    {
        std::string path;
        std::string cmdLine;
        auto dkpPath = getEnv("DEVKITPRO");
        if (!dkpPath.empty())
        {
            // DevkitPro found. We assume the gbalzss executable is there...
#ifdef WIN32
            path = dkpPath + "\\tools\\bin\\gbalzss.exe";
            cmdLine = path;
#else
            path = dkpPath + "/tools/bin/gbalzss";
            cmdLine = path + " 2> /dev/null";
#endif
            // try to execute gbalzss and see if it works
        }
        else
        {
            // DevkitPro not found. See if we can call gbalzss anyway
#ifdef WIN32
            path = "gbalzss.exe";
            cmdLine = path;
#else
            path = "gbalzss";
            cmdLine = path + " 2> /dev/null";
#endif
            // try to execute gbalzss and see if it works
        }
        return (WEXITSTATUS(system(cmdLine.c_str())) == 1 ? path : "");
    }

    std::vector<uint8_t> compressLzss(const std::vector<uint8_t> &data, bool vramCompatible, bool lz11Compression)
    {
        if (GbaLzssPath.empty())
        {
            GbaLzssPath = findGbalzss();
        }
        REQUIRE(!GbaLzssPath.empty(), std::runtime_error, "No gbalzss executable found");
        std::vector<uint8_t> result;
// get process id
#ifdef _MSC_VER
        auto processId = _getpid();
#else
        auto processId = getpid();
#endif
        // write temporary file
        const std::string tempFileName = std::filesystem::temp_directory_path().generic_string() + "/compress_" + std::to_string(processId) + ".tmp";
        std::ofstream outFile(tempFileName, std::ios::binary | std::ios::out);
        if (outFile.is_open())
        {
            outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
            outFile.close();
            // run compressor
            const std::string cmdLine = GbaLzssPath + (vramCompatible ? " --vram" : "") + (lz11Compression ? " --lz11" : "") + " e " + tempFileName + " " + tempFileName;
            if (WEXITSTATUS(std::system(cmdLine.c_str())) == 0)
            {
                // read temporary file
                std::ifstream inFile(tempFileName, std::ios::binary | std::ios::in);
                if (inFile.is_open())
                {
                    // find file size
                    inFile.seekg(0, std::ios::end);
                    size_t fileSize = inFile.tellg();
                    inFile.seekg(0, std::ios::beg);
                    // read data
                    result.resize(fileSize);
                    inFile.read(reinterpret_cast<char *>(result.data()), fileSize);
                    inFile.close();
                }
                else
                {
                    THROW(std::runtime_error, "Failed to read temporary file");
                }
            }
            else
            {
                THROW(std::runtime_error, "Failed to run compressor");
            }
            std::filesystem::remove(tempFileName);
        }
        else
        {
            THROW(std::runtime_error, "Failed to write temporary file");
        }
        return result;
    }

}

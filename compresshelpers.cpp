#include "compresshelpers.h"

#include "exception.h"
#include "filehelpers.h"

#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif

//#include <iostream>

namespace Compression
{

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
        const std::string tempFileName = stdfs::temp_directory_path().generic_string() + "/compress_" + std::to_string(processId) + ".tmp";
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
            stdfs::remove(tempFileName);
        }
        else
        {
            THROW(std::runtime_error, "Failed to write temporary file");
        }
        return result;
    }

    std::vector<uint8_t> compressRLE(const std::vector<uint8_t> &data, bool /*vramCompatible*/)
    {
        constexpr int32_t MinRepeatLength = 3;
        constexpr int32_t MaxRepeatLength = ((1 << 7) - 1) + MinRepeatLength;
        constexpr int32_t MaxCopyLength = ((1 << 7) - 1) + 1;
        std::vector<uint8_t> result;
        auto runStart = data.cbegin();
        auto runEnd = std::next(data.cbegin());
        std::size_t runLength = 1;
        bool isRepetition = false;
        //std::cout << data.size() << std::endl;
        while (runEnd < data.cend())
        {
            // find repetition
            while (runEnd < data.cend() && *runStart == *runEnd && runLength < MaxRepeatLength)
            {
                isRepetition = true;
                runEnd = std::next(runEnd);
                runLength++;
            }
            if (isRepetition)
            {
                // a repetition was found and has ended
                if (runLength >= MinRepeatLength)
                {
                    // store repetition if longer than min repetition length
                    result.push_back((1 << 7) + (runLength - MinRepeatLength));
                    result.push_back(*runStart);
                    runStart = std::next(runStart, runLength);
                    runEnd = runStart;
                    runLength = 1;
                }
                // else, treat it as a verbatim copy run
                isRepetition = false;
            }
            if (runLength >= MaxCopyLength)
            {
                // the max copy length has been reached. store verbatim copy
                result.push_back(runLength - 1);
                std::copy(runStart, runEnd, std::back_inserter(result));
                runStart = std::next(runStart, runLength);
                runEnd = runStart;
                runLength = 1;
            }
            runEnd = std::next(runEnd);
        }
        // we might have some leftover verbatim bytes here. store them
        if (runLength > 1)
        {
            result.push_back(runLength - 1);
            std::copy(runStart, runEnd, std::back_inserter(result));
        }
        return result;
    }

}

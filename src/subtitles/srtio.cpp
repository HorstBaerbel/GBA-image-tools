#include "srtio.h"

#include "exception.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <regex>

namespace IO
{

    static const char *SrtWhiteSpace = " \t\n\r\f\v";
    static const std::regex SrtTimeRegEx("(\\d\\d):(\\d\\d):(\\d\\d),(\\d\\d\\d)\\s-->\\s(\\d\\d):(\\d\\d):(\\d\\d),(\\d\\d\\d)");

    auto toTimeInS(const std::string &line, const std::string &sH, const std::string &sM, const std::string &sS, const std::string &sMs) -> double
    {
        REQUIRE(!sH.empty() && !sM.empty() && !sS.empty() && !sMs.empty(), std::runtime_error, "Bad time format in: " << line);
        const auto hours = std::stoi(sH);
        const auto minutes = std::stoi(sM);
        const auto seconds = std::stoi(sS);
        const auto miliseconds = std::stoi(sMs);
        return hours * 3600.0 + minutes * 60.0 + seconds + miliseconds / 1000.0;
    }

    auto SRT::readSRT(const std::string &filePath) -> std::vector<Subtitles::Frame>
    {
        REQUIRE(!filePath.empty(), std::runtime_error, "filePath must contain a file name");
        // open input file
        auto fileSrt = std::ifstream(filePath, std::ios::in);
        REQUIRE(fileSrt.is_open() && !fileSrt.fail(), std::runtime_error, "Failed to open " << filePath << " for reading");
        // read everything into lines buffer
        std::vector<std::string> lines;
        for (std::string line; std::getline(fileSrt, line);)
        {
            line.erase(line.find_last_not_of(SrtWhiteSpace) + 1); // trim right
            line.erase(0, line.find_first_not_of(SrtWhiteSpace)); // trim left
            lines.push_back(line);
        }
        fileSrt.close();
        REQUIRE(!lines.empty(), std::runtime_error, "No content in " << filePath);
        // now match lines to subtitles
        std::vector<Subtitles::Frame> subtitles;
        auto linesIt = lines.cbegin();
        while (linesIt != lines.cend())
        {
            // we expect the following format:
            // INDEX_NUMBER
            // START_TIME --> END_TIME
            // TEXT
            // (opt. TEXT)
            // blank line
            const auto index = std::stoi(*linesIt);
            REQUIRE(index > 0, std::runtime_error, "Bad subtitle index: " << *linesIt);
            ++linesIt;
            REQUIRE(linesIt != lines.cend(), std::runtime_error, "Unexpected end of subtitles file");
            std::smatch time_match;
            REQUIRE(std::regex_match(*linesIt, time_match, SrtTimeRegEx), std::runtime_error, "Failed to find start / end time in: " << *linesIt);
            REQUIRE(time_match.size() == 9, std::runtime_error, "Failed to find start / end time in: " << *linesIt);
            const auto startTime = toTimeInS(*linesIt, time_match[1].str(), time_match[2].str(), time_match[3].str(), time_match[4].str());
            const auto endTime = toTimeInS(*linesIt, time_match[5].str(), time_match[6].str(), time_match[7].str(), time_match[8].str());
            REQUIRE(endTime > startTime, std::runtime_error, "Subtitle start time must be < end time: " << *linesIt);
            ++linesIt;
            REQUIRE(linesIt != lines.cend(), std::runtime_error, "Unexpected end of subtitles file");
            std::string text;
            for (; linesIt != lines.cend() && !linesIt->empty(); ++linesIt)
            {
                const auto &line = *linesIt;
                text = text.empty() ? line : text + "\n" + line;
            }
            // std::cout << "Subtitle #" << subtitles.size() << ": " << startTime << "s -> " << endTime << "s \"" << text << "\"" << std::endl;
            subtitles.push_back({static_cast<uint32_t>(subtitles.size()), startTime, endTime, text});
            if (linesIt == lines.cend())
            {
                break;
            }
            linesIt = linesIt != lines.cend() ? std::next(linesIt) : linesIt;
        }
        REQUIRE(!subtitles.empty(), std::runtime_error, "No subtitles found in " << filePath);
        return subtitles;
    }
}

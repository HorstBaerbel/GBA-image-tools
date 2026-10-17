#include "testmacros.h"

#include "subtitles/srtio.h"
#include "subtitles/subtitlesstructs.h"

#include <cstdio>
#include <fstream>

#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace IO;
using namespace Subtitles;

TEST_SUITE("SRT IO")

// Helper to write a temporary SRT file
static std::string writeTempSRT(const std::string &content)
{
    const std::string filePath = "/tmp/test_srt_" + std::to_string(std::hash<std::string>{}(content)) + ".srt";
    std::ofstream file(filePath);
    file << content;
    file.close();
    return filePath;
}

TEST_CASE("Read simple SRT file")
{
    const std::string content =
        "1\n"
        "00:00:01,000 --> 00:00:03,500\n"
        "Hello World\n"
        "\n"
        "2\n"
        "00:00:04,000 --> 00:00:06,000\n"
        "Second subtitle\n"
        "\n";

    auto filePath = writeTempSRT(content);
    auto subtitles = SRT::readSRT(filePath);

    CATCH_REQUIRE(subtitles.size() == 2);

    // First subtitle
    CATCH_REQUIRE(subtitles[0].index == 0);
    CATCH_REQUIRE_THAT(subtitles[0].startTimeS, Catch::Matchers::WithinAbs(1.0, 0.0001));
    CATCH_REQUIRE_THAT(subtitles[0].endTimeS, Catch::Matchers::WithinAbs(3.5, 0.0001));
    CATCH_REQUIRE(subtitles[0].text == "Hello World");

    // Second subtitle
    CATCH_REQUIRE(subtitles[1].index == 1);
    CATCH_REQUIRE_THAT(subtitles[1].startTimeS, Catch::Matchers::WithinAbs(4.0, 0.0001));
    CATCH_REQUIRE_THAT(subtitles[1].endTimeS, Catch::Matchers::WithinAbs(6.0, 0.0001));
    CATCH_REQUIRE(subtitles[1].text == "Second subtitle");

    std::remove(filePath.c_str());
}

TEST_CASE("Read SRT with multi-line text")
{
    const std::string content =
        "1\n"
        "00:00:01,000 --> 00:00:03,000\n"
        "Line one\n"
        "Line two\n"
        "Line three\n"
        "\n";

    auto filePath = writeTempSRT(content);
    auto subtitles = SRT::readSRT(filePath);

    CATCH_REQUIRE(subtitles.size() == 1);
    CATCH_REQUIRE(subtitles[0].text == "Line one\nLine two\nLine three");

    std::remove(filePath.c_str());
}

TEST_CASE("Read SRT with hours in timestamp")
{
    const std::string content =
        "1\n"
        "01:02:03,456 --> 01:02:05,789\n"
        "Test\n"
        "\n";

    auto filePath = writeTempSRT(content);
    auto subtitles = SRT::readSRT(filePath);

    CATCH_REQUIRE(subtitles.size() == 1);
    // 1h 2m 3.456s = 3600 + 120 + 3.456 = 3723.456
    CATCH_REQUIRE_THAT(subtitles[0].startTimeS, Catch::Matchers::WithinAbs(3723.456, 0.0001));
    // 1h 2m 5.789s = 3600 + 120 + 5.789 = 3725.789
    CATCH_REQUIRE_THAT(subtitles[0].endTimeS, Catch::Matchers::WithinAbs(3725.789, 0.0001));

    std::remove(filePath.c_str());
}

TEST_CASE("Read SRT with minutes only")
{
    const std::string content =
        "1\n"
        "00:05:30,000 --> 00:05:32,500\n"
        "Five and a half minutes\n"
        "\n";

    auto filePath = writeTempSRT(content);
    auto subtitles = SRT::readSRT(filePath);

    CATCH_REQUIRE(subtitles.size() == 1);
    // 5m 30s = 330s
    CATCH_REQUIRE_THAT(subtitles[0].startTimeS, Catch::Matchers::WithinAbs(330.0, 0.0001));
    // 5m 32.5s = 332.5s
    CATCH_REQUIRE_THAT(subtitles[0].endTimeS, Catch::Matchers::WithinAbs(332.5, 0.0001));

    std::remove(filePath.c_str());
}

TEST_CASE("Read SRT with multiple subtitles")
{
    const std::string content =
        "1\n"
        "00:00:00,000 --> 00:00:02,000\n"
        "First\n"
        "\n"
        "2\n"
        "00:00:02,000 --> 00:00:04,000\n"
        "Second\n"
        "\n"
        "3\n"
        "00:00:04,000 --> 00:00:06,000\n"
        "Third\n"
        "\n";

    auto filePath = writeTempSRT(content);
    auto subtitles = SRT::readSRT(filePath);

    CATCH_REQUIRE(subtitles.size() == 3);
    CATCH_REQUIRE(subtitles[0].text == "First");
    CATCH_REQUIRE(subtitles[1].text == "Second");
    CATCH_REQUIRE(subtitles[2].text == "Third");

    std::remove(filePath.c_str());
}

TEST_CASE("Read SRT with extra blank lines")
{
    const std::string content =
        "1\n"
        "00:00:01,000 --> 00:00:03,000\n"
        "Hello\n"
        "\n"
        "\n"
        "\n"
        "2\n"
        "00:00:04,000 --> 00:00:06,000\n"
        "World\n"
        "\n";

    auto filePath = writeTempSRT(content);
    auto subtitles = SRT::readSRT(filePath);

    CATCH_REQUIRE(subtitles.size() == 2);
    CATCH_REQUIRE(subtitles[0].text == "Hello");
    CATCH_REQUIRE(subtitles[1].text == "World");

    std::remove(filePath.c_str());
}

TEST_CASE("Read SRT with whitespace trimming")
{
    const std::string content =
        "  1  \n"
        "  00:00:01,000 --> 00:00:03,000  \n"
        "   Hello World   \n"
        "\n";

    auto filePath = writeTempSRT(content);
    auto subtitles = SRT::readSRT(filePath);

    CATCH_REQUIRE(subtitles.size() == 1);
    CATCH_REQUIRE(subtitles[0].text == "Hello World");

    std::remove(filePath.c_str());
}

TEST_CASE("Non-existent file throws")
{
    CATCH_REQUIRE_THROWS_AS(
        SRT::readSRT("/nonexistent/file.srt"),
        std::runtime_error);
}

TEST_CASE("Empty file path throws")
{
    CATCH_REQUIRE_THROWS_AS(
        SRT::readSRT(""),
        std::runtime_error);
}

TEST_CASE("Empty file throws")
{
    const std::string content = "";
    auto filePath = writeTempSRT(content);
    CATCH_REQUIRE_THROWS_AS(
        SRT::readSRT(filePath),
        std::runtime_error);

    std::remove(filePath.c_str());
}

TEST_CASE("Invalid time format throws")
{
    const std::string content =
        "1\n"
        "INVALID_TIME --> 00:00:03,000\n"
        "Hello\n"
        "\n";

    auto filePath = writeTempSRT(content);
    CATCH_REQUIRE_THROWS_AS(
        SRT::readSRT(filePath),
        std::runtime_error);

    std::remove(filePath.c_str());
}

TEST_CASE("End time before start time throws")
{
    const std::string content =
        "1\n"
        "00:00:05,000 --> 00:00:03,000\n"
        "Hello\n"
        "\n";

    auto filePath = writeTempSRT(content);
    CATCH_REQUIRE_THROWS_AS(
        SRT::readSRT(filePath),
        std::runtime_error);

    std::remove(filePath.c_str());
}

TEST_CASE("Milliseconds precision")
{
    const std::string content =
        "1\n"
        "00:00:01,123 --> 00:00:01,456\n"
        "Short subtitle\n"
        "\n";

    auto filePath = writeTempSRT(content);
    auto subtitles = SRT::readSRT(filePath);

    CATCH_REQUIRE(subtitles.size() == 1);
    CATCH_REQUIRE_THAT(subtitles[0].startTimeS, Catch::Matchers::WithinAbs(1.123, 0.0001));
    CATCH_REQUIRE_THAT(subtitles[0].endTimeS, Catch::Matchers::WithinAbs(1.456, 0.0001));

    std::remove(filePath.c_str());
}

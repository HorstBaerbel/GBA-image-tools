#include "testmacros.h"

#include "color/colorformat.h"
#include "statistics/statistics.h"

#include <thread>
#include <vector>

using namespace Statistics;

TEST_SUITE("Statistics")

TEST_CASE("Frame setValue and getValue")
{
    Frame frame;
    frame.setValue("test", 42.0);
    CATCH_REQUIRE(frame.getValue("test") == 42.0);
}

TEST_CASE("Frame setValue with index")
{
    Frame frame;
    frame.setValue("test", 1.0, 0);
    frame.setValue("test", 2.0, 1);
    frame.setValue("test", 3.0, 2);
    CATCH_REQUIRE(frame.getValue("test", 0) == 1.0);
    CATCH_REQUIRE(frame.getValue("test", 1) == 2.0);
    CATCH_REQUIRE(frame.getValue("test", 2) == 3.0);
}

TEST_CASE("Frame setValue overwrites existing value")
{
    Frame frame;
    frame.setValue("test", 10.0);
    frame.setValue("test", 20.0);
    CATCH_REQUIRE(frame.getValue("test") == 20.0);
}

TEST_CASE("Frame getValue non-existent returns 0")
{
    Frame frame;
    CATCH_REQUIRE(frame.getValue("nonexistent") == 0.0);
}

TEST_CASE("Frame getValue index out of range returns 0")
{
    Frame frame;
    frame.setValue("test", 5.0, 0);
    // Index 5 doesn't exist
    CATCH_REQUIRE(frame.getValue("test", 5) == 0.0);
}

TEST_CASE("Frame incValue creates new value")
{
    Frame frame;
    frame.incValue("counter");
    CATCH_REQUIRE(frame.getValue("counter") == 1.0);
    frame.incValue("counter");
    CATCH_REQUIRE(frame.getValue("counter") == 2.0);
    frame.incValue("counter", 5.0);
    CATCH_REQUIRE(frame.getValue("counter") == 7.0);
}

TEST_CASE("Frame incValue with index")
{
    Frame frame;
    frame.incValue("counter", 1.0, 0);
    frame.incValue("counter", 2.0, 1);
    frame.incValue("counter", 3.0, 2);
    CATCH_REQUIRE(frame.getValue("counter", 0) == 1.0);
    CATCH_REQUIRE(frame.getValue("counter", 1) == 2.0);
    CATCH_REQUIRE(frame.getValue("counter", 2) == 3.0);
}

TEST_CASE("Frame getValues returns all values")
{
    Frame frame;
    frame.setValue("a", 1.0);
    frame.setValue("b", 2.0);
    frame.setValue("c", 3.0, 0);
    frame.setValue("c", 4.0, 1);
    auto values = frame.getValues();
    CATCH_REQUIRE(values.size() == 3);
    CATCH_REQUIRE(values.at("a").size() == 1);
    CATCH_REQUIRE(values.at("a")[0] == 1.0);
    CATCH_REQUIRE(values.at("b").size() == 1);
    CATCH_REQUIRE(values.at("b")[0] == 2.0);
    CATCH_REQUIRE(values.at("c").size() == 2);
    CATCH_REQUIRE(values.at("c")[0] == 3.0);
    CATCH_REQUIRE(values.at("c")[1] == 4.0);
}

TEST_CASE("Frame setImage and getImages")
{
    Frame frame;
    std::vector<uint8_t> imageData = {0x12, 0x34, 0x56, 0x78};
    frame.setImage("img", imageData, Color::Format::RGB565, 2, 2);
    auto images = frame.getImages();
    CATCH_REQUIRE(images.size() == 1);
    CATCH_REQUIRE(images.at("img").image == imageData);
    CATCH_REQUIRE(images.at("img").colorFormat == Color::Format::RGB565);
    CATCH_REQUIRE(images.at("img").width == 2);
    CATCH_REQUIRE(images.at("img").height == 2);
}

TEST_CASE("Frame setImage move semantics")
{
    Frame frame;
    std::vector<uint8_t> imageData = {0xAA, 0xBB, 0xCC};
    frame.setImage("img", std::move(imageData), Color::Format::RGB888, 3, 1);
    auto images = frame.getImages();
    CATCH_REQUIRE(images.size() == 1);
    CATCH_REQUIRE(images.at("img").image == (std::vector<uint8_t>{0xAA, 0xBB, 0xCC}));
}

TEST_CASE("Frame multiple images")
{
    Frame frame;
    frame.setImage("img1", std::vector<uint8_t>{1, 2}, Color::Format::RGB565, 1, 1);
    frame.setImage("img2", std::vector<uint8_t>{3, 4}, Color::Format::RGB888, 1, 1);
    auto images = frame.getImages();
    CATCH_REQUIRE(images.size() == 2);
    CATCH_REQUIRE(images.at("img1").colorFormat == Color::Format::RGB565);
    CATCH_REQUIRE(images.at("img2").colorFormat == Color::Format::RGB888);
}

TEST_CASE("Container addFrame")
{
    Container container;
    auto frame1 = container.addFrame();
    auto frame2 = container.addFrame();
    CATCH_REQUIRE(frame1 != nullptr);
    CATCH_REQUIRE(frame2 != nullptr);
    CATCH_REQUIRE(frame1 != frame2);
    auto frames = container.getFrames();
    CATCH_REQUIRE(frames.size() == 2);
}

TEST_CASE("Container getFrames empty")
{
    Container container;
    auto frames = container.getFrames();
    CATCH_REQUIRE(frames.empty());
}

TEST_CASE("Free function setValue with valid frame")
{
    auto frame = std::make_shared<Frame>();
    setValue(frame, "test", 99.0);
    CATCH_REQUIRE(frame->getValue("test") == 99.0);
}

TEST_CASE("Free function setValue with nullptr")
{
    // Should not throw
    setValue(nullptr, "test", 99.0);
    CATCH_REQUIRE(true);
}

TEST_CASE("Free function incValue with valid frame")
{
    auto frame = std::make_shared<Frame>();
    incValue(frame, "counter");
    incValue(frame, "counter");
    CATCH_REQUIRE(frame->getValue("counter") == 2.0);
}

TEST_CASE("Free function incValue with nullptr")
{
    // Should not throw
    incValue(nullptr, "counter");
    CATCH_REQUIRE(true);
}

TEST_CASE("Free function setImage with valid frame")
{
    auto frame = std::make_shared<Frame>();
    setImage(frame, "img", std::vector<uint8_t>{1, 2, 3}, Color::Format::RGB565, 3, 1);
    auto images = frame->getImages();
    CATCH_REQUIRE(images.size() == 1);
    CATCH_REQUIRE(images.at("img").image == (std::vector<uint8_t>{1, 2, 3}));
}

TEST_CASE("Free function setImage with nullptr")
{
    // Should not throw
    setImage(nullptr, "img", std::vector<uint8_t>{1, 2}, Color::Format::RGB565, 1, 1);
    CATCH_REQUIRE(true);
}

TEST_CASE("Thread safety - concurrent incValue")
{
    auto frame = std::make_shared<Frame>();
    const int numThreads = 4;
    const int incrementsPerThread = 100;
    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back([&frame]()
                             {
            for (int i = 0; i < incrementsPerThread; ++i) {
                incValue(frame, "counter");
            } });
    }
    for (auto &thread : threads)
    {
        thread.join();
    }
    CATCH_REQUIRE(frame->getValue("counter") == numThreads * incrementsPerThread);
}

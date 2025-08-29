#pragma once

#include <chrono>
#include <cmath>
#include <functional>
#include <future>
#include <iostream>

#define TIME_SECTION_START(name) \
    auto name##StartTime = std::chrono::high_resolution_clock::now();

#define TIME_SECTION_END(name)                                                                                     \
    {                                                                                                              \
        auto name##EndTime = std::chrono::high_resolution_clock::now();                                            \
        auto duration_uS = std::chrono::duration_cast<std::chrono::microseconds>(name##EndTime - name##StartTime); \
        std::cout << #name << " took " << duration_uS << " us" << std::endl;                                       \
    }

/// @brief Timer class with milisecond precision
class Timer
{
public:
    Timer() = default;

    template <class callable, class... arguments>
    auto start(double intervalMs, callable &&f, arguments &&...args) -> void
    {
        std::function<typename std::result_of<callable(arguments...)>::type()> func(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));
        m_thread = std::thread([intervalMs, this, func]()
                               {
            // store next interval
            double nextIntervalMs = intervalMs;
            while (!this->m_quit)
            {
                // round this interval to ms and calculate error to required value
                int64_t thisIntervalMs = std::round(nextIntervalMs);
                const double thisIntervalErrorMs = nextIntervalMs - thisIntervalMs;
                // sleep interval
                std::this_thread::sleep_for(std::chrono::milliseconds(thisIntervalMs));
                // run timer function and time how long it takes
                const auto startTime = std::chrono::high_resolution_clock::now();
                func();
                const std::chrono::duration<double, std::milli> funcRuntimeMs = std::chrono::high_resolution_clock::now() - startTime;
                // calculate new interval base on required value, plus error, minus the time it took to run func()
                nextIntervalMs = intervalMs + thisIntervalErrorMs - funcRuntimeMs.count();
                if (nextIntervalMs < 0)
                {
                    // we're skipping frames
                    nextIntervalMs = 0;
                }
            } });
        m_thread.detach();
    }

    auto stop() -> void
    {
        if (!m_quit)
        {
            m_quit = true;
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }
    }

    ~Timer()
    {
        stop();
    }

private:
    std::atomic<bool> m_quit = false;
    std::thread m_thread;
};

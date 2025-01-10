#pragma once

#include <chrono>
#include <iostream>

#define TIME_SECTION_START(name) \
    auto name##StartTime = std::chrono::high_resolution_clock::now();

#define TIME_SECTION_END(name)                                                                                     \
    {                                                                                                              \
        auto name##EndTime = std::chrono::high_resolution_clock::now();                                            \
        auto duration_uS = std::chrono::duration_cast<std::chrono::microseconds>(name##EndTime - name##StartTime); \
        std::cout << #name << " took " << duration_uS << " us" << std::endl;                                       \
    }

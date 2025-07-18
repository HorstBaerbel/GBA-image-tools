#pragma once

#include "audio/audiostructs.h"
#include "exception.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace IO
{

    class File
    {
    public:
        /// @brief Write audio data to a WAV file
        /// @param info Information about the audio sample data
        /// @param samples Audio sample data
        /// @param folder File output folder. If empty, only fileName will be used, if filled
        /// @param fileName Optional. If passed used as a file name, if empty info.fileName is used
        /// @note Will create necessary directories if not found
        static auto writeAudio(const Audio::FrameInfo &info, const Audio::SampleData &samples, const std::string &folder, const std::string &fileName = "") -> void;
    };

}

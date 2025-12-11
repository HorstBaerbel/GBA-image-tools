#pragma once

#include "audio_codec/adpcm.h"
#include "audiostructs.h"
#include "processingtype.h"
#include "resampler.h"
#include "samplebuffer.h"
#include "statistics/statistics.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace Audio
{
    class Processing
    {
    public:
        /// @brief Variable parameters for processing step
        using Parameter = std::variant<bool, int32_t, uint32_t, double, ChannelFormat, SampleFormat>;

        /// @brief Construct an audio processing pipeline
        Processing() = default;

        /// @brief Add a processing step and its parameters
        /// @param type Processing type
        /// @param parameters Parameters to pass to processing
        /// @param decodeRelevant If true the processing type is recorded for a call to getDecodingSteps().
        /// @param addStatistics The step should output statistics to the statistics container
        void addStep(ProcessingType type, const std::vector<Parameter> &parameters, bool decodeRelevant = false, bool addStatistics = false);

        /// @brief Get current # of steps in processing pipeline
        std::size_t nrOfSteps() const;

        /// @brief Remove all processing steps. Will also reset()
        void clearSteps();

        /// @brief Clear the internal state of all processing steps. Call to reset state if you want to run the processing pipeline multiple times
        void reset();

        /// @brief Get human-readable description for processing steps in pipeline
        std::string getProcessingDescription(const std::string &seperator = ", ");

        /// @brief Run processing steps in pipeline on single frame. Used for processing a stream of audio frames
        /// @param frame Input data and file name
        /// @param flushBuffers Pass true to dump queued data from internal buffers to the output frame
        /// @param statistics Statistics container to write statistics to
        /// @return Returns a frame when the pipeline can output one. Due to buffering this might not be 1:1.
        /// Call the function until even after flushing the stream with an empty frame nor frames are returned: !processStream(Audio::Frame(), true);
        std::optional<Frame> processStream(const Frame &frame, bool flushBuffers = false, Statistics::Container::SPtr statistics = nullptr);

        /// @brief Get number of frames received in processStream()
        /// @return Number of frames received in processStream()
        uint32_t nrOfInputFrames() const;

        /// @brief Get number of frames returned from processStream()
        /// @return Number of frames returned from processStream()
        uint32_t nrOfOutputFrames() const;

        /// @brief Get number of output samples returned from processStream()
        /// @return Number of output samples returned from processStream()
        uint32_t nrOfOutputSamples() const;

        /// @brief Get max. memory needed to keep intermediate processing results
        /// @return Max. memory needed to keep intermediate processing results
        uint32_t outputMaxMemoryNeeded() const;

        /// @brief Get output frame info of last frame output from processStream()
        /// @return Output frame info of last frame output from processStream()
        FrameInfo outputFrameInfo() const;

        /// @brief Get the processing needed to decode the data (steps reversed). These might not be all steps added with addStep()
        /// @return Processing steps needed to decode the data
        std::vector<ProcessingType> getDecodingSteps() const;

    private:
        // --- audio conversion functions ------------------------------------

        /// @brief Resample audio data to a different sample rate, bit depth or channel format
        /// @param parameters:
        /// - Channel format as ChannelFormat
        /// - Sample rate as uint32_t
        /// - Sample format as SampleFormat
        /// @param flushBuffers Pass true to dump queued data from internal buffers to the output frame
        /// @param statistics Statistics container to write statistics to
        /// @note Will return frame as-is if it is already in the requested format
        /// @return Returns frame in the new format
        static std::optional<Frame> resample(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics);

        /// @brief Buffer and re-package audio data to a different frame size
        /// @param parameters:
        /// - Samples per channel as double
        /// - Samples count modulo value (e.g. 16 for GBA, 4 for NDS)
        /// @param flushBuffers Pass true to dump queued data from internal buffers to the output frame
        /// @param statistics Statistics container to write statistics to
        /// @return Returns frame in the new size
        static std::optional<Frame> repackage(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics);

        // --- compression functions -------------------------------------------------------------

        /// @brief Compress audio data using LZSS variant 10h
        /// @param parameters:
        /// - Flag for VRAM-compatible compression as bool. Pass true to turn on
        /// @param flushBuffers Pass true to dump queued data from internal buffers to the output frame
        /// @param statistics Statistics container to write statistics to
        /// @return Compressed frame
        static std::optional<Frame> compressLZSS_10(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics);

        /// @brief Compress audio data using rANS variant 50h
        /// @param parameters: none
        /// @param flushBuffers Pass true to dump queued data from internal buffers to the output frame
        /// @param statistics Statistics container to write statistics to
        /// @return Compressed frame
        static std::optional<Frame> compressRANS_50(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics);

        /// @brief Compress audio data using RLE
        /// @param parameters:
        /// - Flag for VRAM-compatible compression as bool. Pass true to turn on
        /// @param flushBuffers Pass true to dump queued data from internal buffers to the output frame
        /// @param statistics Statistics container to write statistics to
        /// @return Compressed frame
        static std::optional<Frame> compressRLE(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics);

        /// @brief Encode audio data as 4-bit ADPCM data
        /// @param parameters: Unused
        /// @param flushBuffers Pass true to dump queued data from internal buffers to the output frame
        /// @param statistics Statistics container to write statistics to
        /// @return Compressed frame
        static std::optional<Frame> compressADPCM(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics);

        // --- misc conversion functions ------------------------------------------------------------------------

        /// @brief Convert convert audio data to raw data
        /// @param parameters:
        /// - Flag for planar -> interleaved sample conversion as bool. Pass true to convert samples from (L0 L1 ... R0 R1 ...) -> (L0 R0 L1 R1 ...)
        /// @return Raw frame
        /// @note Will do nothing if the audio data is already in raw format
        static std::optional<Frame> convertSamplesToRaw(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics);

        /// @brief Fill up audio data with 0s to a multiple of N bytes
        /// @return Raw frame
        /// @param parameters "Modulo value" as uint32_t. The audio data will be padded to a multiple of this
        static std::optional<Frame> padAudioData(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics);

        struct ProcessingStep
        {
            ProcessingType type;               // Type of processing operation applied
            std::vector<Parameter> parameters; // Input parameters for operation
            bool decodeRelevant = false;       // If processing information is needed for decoding
            bool addStatistics = false;        // If operation statistics should be written to
            std::vector<uint8_t> state;        // The input / output state for stateful operations
        };
        std::vector<ProcessingStep> m_steps; // Processing steps used in pipeline

        uint32_t m_nrOfInputFrames = 0;               // Number of frames input into processing
        uint32_t m_nrOfOutputFrames = 0;              // Number of frames output from processing
        uint32_t m_nrOfOutputSamples = 0;             // Number of samples output from processing
        uint32_t m_outputMaxMemoryNeeded = 0;         // Max. buffer size needed for decompression
        FrameInfo m_outputFrameInfo;                  // Frame info from last frame output from processStream()
        std::shared_ptr<Resampler> m_resampler;       // Audio resampler using FFmpeg
        std::shared_ptr<SampleBuffer> m_sampleBuffer; // Audio buffer for repackaging samples
        double m_sampleDeltaPrevFrame = 0;            // Number of audio samples per channel last frame in comparison to requested audioSamplesPerFrame
        std::shared_ptr<Adpcm> m_codecAdpcm;          // ADPCM compressor

        enum class OperationType
        {
            Convert,      // Converts 1 data input into 1 data output
            ConvertState, // Converts 1 data input + state into 1 data output
        };

        using ConvertFunc = std::function<std::optional<Frame>(Processing &processing, const Frame &, const std::vector<Parameter> &, bool, Statistics::Frame::SPtr statistics)>;
        using ConvertStateFunc = std::function<std::optional<Frame>(Processing &processing, const Frame &, const std::vector<Parameter> &, std::vector<uint8_t> &, bool, Statistics::Frame::SPtr statistics)>;
        using FunctionType = std::variant<ConvertFunc, ConvertStateFunc>;

        struct ProcessingFunc
        {
            std::string description; // Processing operation description
            OperationType type;      // Of what type the operation is
            FunctionType func;       // Actual processing function
        };
        static const std::map<ProcessingType, ProcessingFunc> ProcessingFunctions;
    };
}
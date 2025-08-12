#include "audioprocessing.h"

#include "audiohelpers.h"
#include "compression/lzss.h"
#include "exception.h"
#include "processing/datahelpers.h"
#include "processing/varianthelpers.h"

#include <iomanip>
#include <iostream>

namespace Audio
{

    const std::map<ProcessingType, Processing::ProcessingFunc>
        Processing::ProcessingFunctions = {
            {ProcessingType::Resample, {"resample", OperationType::Convert, FunctionType(resample)}},
            {ProcessingType::Repackage, {"repackage", OperationType::Convert, FunctionType(repackage)}},
            {ProcessingType::CompressLZ10, {"compress LZ10", OperationType::Convert, FunctionType(compressLZ10)}},
            //{ProcessingType::CompressRLE, {"compress RLE", OperationType::Convert, FunctionType(compressRLE)}},
            {ProcessingType::CompressADPCM, {"ADPCM compression", OperationType::Convert, FunctionType(compressADPCM)}},
            {ProcessingType::ConvertSamplesToRaw, {"data to raw", OperationType::Convert, FunctionType(convertSamplesToRaw)}},
            {ProcessingType::PadAudioData, {"pad pixel data", OperationType::Convert, FunctionType(padAudioData)}}};

    // ----------------------------------------------------------------------------

    std::optional<Frame> Processing::resample(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<ChannelFormat, uint32_t, SampleFormat>(parameters)), std::runtime_error, "resample expects a channel format, sample rate and sample format parameter");
        const auto outChannelFormat = VariantHelpers::getValue<ChannelFormat, 0>(parameters);
        const auto outSampleRateHz = VariantHelpers::getValue<uint32_t, 1>(parameters);
        const auto outSampleFormat = VariantHelpers::getValue<SampleFormat, 2>(parameters);
        if (processing.m_resampler == nullptr)
        {
            processing.m_resampler = std::make_shared<Audio::Resampler>(frame.info.channelFormat, frame.info.sampleRateHz, outChannelFormat, outSampleRateHz, outSampleFormat);
        }
        return processing.m_resampler->resample(frame, flushBuffers);
    }

    std::optional<Frame> Processing::repackage(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE((VariantHelpers::hasTypes<double, uint32_t>(parameters)), std::runtime_error, "repackage expects a double sample per frame and uint32_t sample count modulo parameter");
        const auto samplesPerFrame = VariantHelpers::getValue<double, 0>(parameters);
        const auto sampleCountModulo = VariantHelpers::getValue<uint32_t, 1>(parameters);
        // store data in buffer
        if (processing.m_sampleBuffer == nullptr)
        {
            processing.m_sampleBuffer = std::make_shared<SampleBuffer>(frame.info.channelFormat, frame.info.sampleRateHz, frame.info.sampleFormat);
        }
        processing.m_sampleBuffer->push_back(frame);
        if (flushBuffers)
        {
            // flush buffer. check if we have audio samples left
            const auto audioSamplesRemaining = processing.m_sampleBuffer->nrOfSamplesPerChannel();
            if (audioSamplesRemaining > 0)
            {
                auto audioSamplesThisFrame = audioSamplesRemaining;
                // round up to multiple of sampleCountModulo samples
                if (audioSamplesThisFrame % sampleCountModulo != 0)
                {
                    audioSamplesThisFrame += sampleCountModulo - (audioSamplesThisFrame % sampleCountModulo);
                    // push silence to the buffer to extend samples
                    processing.m_sampleBuffer->push_silence(audioSamplesThisFrame - audioSamplesRemaining);
                }
                processing.m_sampleDeltaPrevFrame += audioSamplesThisFrame - samplesPerFrame;
                return processing.m_sampleBuffer->pop_front(audioSamplesThisFrame);
            }
        }
        else
        {
            // get packet from buffer
            auto audioSamplesThisFrame = static_cast<int32_t>(std::ceil(samplesPerFrame - processing.m_sampleDeltaPrevFrame));
            // round up to multiple of sampleCountModulo samples
            if (audioSamplesThisFrame % sampleCountModulo != 0)
            {
                audioSamplesThisFrame += sampleCountModulo - (audioSamplesThisFrame % sampleCountModulo);
            }
            // check if our buffer has this many samples
            if (processing.m_sampleBuffer->nrOfSamplesPerChannel() >= audioSamplesThisFrame)
            {
                processing.m_sampleDeltaPrevFrame += audioSamplesThisFrame - samplesPerFrame;
                return processing.m_sampleBuffer->pop_front(audioSamplesThisFrame);
            }
        }
        return {};
    }

    // ----------------------------------------------------------------------------

    std::optional<Frame> Processing::compressLZ10(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "compressLZ10 expects a bool VRAMcompatible parameter");
        const auto vramCompatible = VariantHelpers::getValue<bool, 0>(parameters);
        // compress data
        auto result = frame;
        result.data = Compression::encodeLZ10(AudioHelpers::toRawData(result.data, result.info.channelFormat), vramCompatible);
        result.info.compressed = true;
        // print statistics
        if (statistics != nullptr)
        {
            const auto ratioPercent = static_cast<double>(AudioHelpers::rawDataSize(result.data) * 100.0 / static_cast<double>(AudioHelpers::rawDataSize(frame.data)));
            std::cout << "LZ10 compression ratio: " << std::fixed << std::setprecision(1) << ratioPercent << "%" << std::endl;
        }
        return result;
    }

    std::optional<Frame> Processing::compressRLE(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "compressRLE expects a bool VRAMcompatible parameter");
        const auto vramCompatible = VariantHelpers::getValue<bool, 0>(parameters);
        // compress data
        auto result = frame;
        // result.data = RLE::encodeRLE(frame.data, vramCompatible);
        result.info.compressed = true;
        return result;
    }

    std::optional<Frame> Processing::compressADPCM(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, bool flushBuffers, Statistics::Frame::SPtr statistics)
    {
        THROW(std::runtime_error, "Not implemented");
    }

    // ----------------------------------------------------------------------------

    std::optional<Frame> Processing::convertSamplesToRaw(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, [[maybe_unused]] bool flushBuffers, Statistics::Frame::SPtr statistics)
    {
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<bool>(parameters), std::runtime_error, "convertSamplesToRaw expects a bool interleaved conversion parameter");
        const auto toInterleaved = VariantHelpers::getValue<bool, 0>(parameters);
        // convert data
        auto result = frame;
        result.data = toInterleaved ? AudioHelpers::toRawInterleavedData(result.data, result.info.channelFormat) : AudioHelpers::toRawData(result.data, result.info.channelFormat);
        return result;
    }

    std::optional<Frame> Processing::padAudioData(Processing &processing, const Frame &frame, const std::vector<Parameter> &parameters, [[maybe_unused]] bool flushBuffers, Statistics::Frame::SPtr statistics)
    {
        REQUIRE(std::holds_alternative<std::vector<uint8_t>>(frame.data), std::runtime_error, "audio data padding is only possible for raw data");
        // get parameter(s)
        REQUIRE(VariantHelpers::hasTypes<uint32_t>(parameters), std::runtime_error, "padPixelData expects a uint32_t pad modulo parameter");
        auto multipleOf = VariantHelpers::getValue<uint32_t, 0>(parameters);
        // pad pixel data
        auto result = frame;
        result.data = DataHelpers::fillUpToMultipleOf(AudioHelpers::toRawData(result.data, result.info.channelFormat), multipleOf);
        return result;
    }

    // ----------------------------------------------------------------------------

    void Processing::addStep(ProcessingType type, const std::vector<Parameter> &parameters, bool prependProcessingInfo, bool addStatistics)
    {
        m_steps.push_back({type, parameters, prependProcessingInfo, addStatistics});
    }

    std::size_t Processing::nrOfSteps() const
    {
        return m_steps.size();
    }

    void Processing::clearSteps()
    {
        reset();
        m_steps.clear();
    }

    void Processing::reset()
    {
        m_nrOfInputFrames = 0;
        m_nrOfOutputFrames = 0;
        m_nrOfOutputSamples = 0;
        m_outputFrameInfo = FrameInfo();
        m_resampler.reset();
        m_sampleBuffer.reset();
        m_outputMaxMemoryNeeded = 0;
        m_sampleDeltaPrevFrame = 0;
        for (std::size_t si = 0; si < m_steps.size(); si++)
        {
            m_steps[si].state.clear();
        }
    }

    std::string Processing::getProcessingDescription(const std::string &seperator)
    {
        std::string result;
        for (std::size_t si = 0; si < m_steps.size(); si++)
        {
            const auto &step = m_steps[si];
            const auto &stepFunc = ProcessingFunctions.find(step.type)->second;
            result += stepFunc.description;
            result += step.parameters.size() > 0 ? " " : "";
            for (std::size_t pi = 0; pi < step.parameters.size(); pi++)
            {
                const auto &p = step.parameters[pi];
                if (std::holds_alternative<bool>(p))
                {
                    result += std::get<bool>(p) ? "true" : "false";
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<int32_t>(p))
                {
                    result += std::to_string(std::get<int32_t>(p));
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<uint32_t>(p))
                {
                    result += std::to_string(std::get<uint32_t>(p));
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<double>(p))
                {
                    result += std::to_string(std::get<double>(p));
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<Audio::ChannelFormat>(p))
                {
                    result += Audio::formatInfo(std::get<Audio::ChannelFormat>(p)).id;
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<Audio::SampleFormat>(p))
                {
                    result += Audio::formatInfo(std::get<Audio::SampleFormat>(p)).id;
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
                else if (std::holds_alternative<std::string>(p))
                {
                    result += std::get<std::string>(p);
                    result += (pi < (step.parameters.size() - 1) ? " " : "");
                }
            }
            result += (si < (m_steps.size() - 1) ? seperator : "");
        }
        return result;
    }

    std::optional<Frame> Processing::processStream(const Frame &frame, bool flushBuffers, Statistics::Container::SPtr statistics)
    {
        m_nrOfInputFrames++;
        bool finalStepFound = false;
        Frame processed = frame;
        processed.nrOfSamples = flushBuffers ? 0 : AudioHelpers::nrOfSamples(processed.data, processed.info.channelFormat);
        auto frameStatistics = statistics != nullptr ? statistics->addFrame() : nullptr;
        for (auto stepIt = m_steps.begin(); stepIt != m_steps.end(); ++stepIt)
        {
            const uint32_t inputSize = AudioHelpers::rawDataSize(processed.data);
            auto stepStatistics = stepIt->addStatistics ? frameStatistics : nullptr;
            auto &stepFunc = ProcessingFunctions.find(stepIt->type)->second;
            if (stepFunc.type == OperationType::Convert)
            {
                auto convertFunc = std::get<ConvertFunc>(stepFunc.func);
                auto result = convertFunc(*this, processed, stepIt->parameters, flushBuffers, stepStatistics);
                // check if we got an output frame, else report to the caller
                if (!result)
                {
                    return {};
                }
                processed = result.value();
            }
            else if (stepFunc.type == OperationType::ConvertState)
            {
                auto convertFunc = std::get<ConvertStateFunc>(stepFunc.func);
                auto result = convertFunc(*this, processed, stepIt->parameters, stepIt->state, flushBuffers, stepStatistics);
                // check if we got an output frame, else report to the caller
                if (!result)
                {
                    return {};
                }
                processed = result.value();
            }
            // we're silently ignoring OperationType::BatchInput ::BatchConvert and ::Reduce operations here
            // record max. memory needed for everything, but the first step
            auto chunkMemoryNeeded = stepIt == m_steps.begin() ? 0 : AudioHelpers::rawDataSize(processed.data) + sizeof(uint32_t);
            processed.info.maxMemoryNeeded = (processed.info.maxMemoryNeeded < chunkMemoryNeeded) ? chunkMemoryNeeded : processed.info.maxMemoryNeeded;
        }
        m_outputMaxMemoryNeeded = m_outputMaxMemoryNeeded < processed.info.maxMemoryNeeded ? processed.info.maxMemoryNeeded : m_outputMaxMemoryNeeded;
        m_nrOfOutputSamples += processed.nrOfSamples;
        m_outputFrameInfo = processed.info;
        processed.index = m_nrOfOutputFrames++;
        return processed;
    }

    uint32_t Processing::nrOfInputFrames() const
    {
        return m_nrOfInputFrames;
    }

    uint32_t Processing::nrOfOutputFrames() const
    {
        return m_nrOfOutputFrames;
    }

    uint32_t Processing::nrOfOutputSamples() const
    {
        return m_nrOfOutputSamples;
    }

    uint32_t Processing::outputMaxMemoryNeeded() const
    {
        return m_outputMaxMemoryNeeded;
    }

    FrameInfo Processing::outputFrameInfo() const
    {
        return m_outputFrameInfo;
    }

    std::vector<ProcessingType> Processing::getDecodingSteps() const
    {
        std::vector<ProcessingType> decodingSteps;
        auto srIt = m_steps.crbegin();
        while (srIt != m_steps.crend())
        {
            if (srIt->decodeRelevant)
            {
                decodingSteps.push_back(srIt->type);
            }
            ++srIt;
        }
        return decodingSteps;
    }
}
#include "ffmpegreader.h"

extern "C"
{
#include <inttypes.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <iostream>
#include <cstring>

#include "exception.h"

namespace Media
{

    static const AVChannelLayout monoLayout = AV_CHANNEL_LAYOUT_MONO;
    static const AVChannelLayout stereoLayout = AV_CHANNEL_LAYOUT_STEREO;

    static AVPixelFormat CorrectDeprecatedPixelFormat(AVPixelFormat format)
    {
        // Fix swscaler deprecated pixel format warning (YUVJ has been deprecated, change pixel format to regular YUV)
        switch (format)
        {
        case AV_PIX_FMT_YUVJ420P:
            return AV_PIX_FMT_YUV420P;
        case AV_PIX_FMT_YUVJ422P:
            return AV_PIX_FMT_YUV422P;
        case AV_PIX_FMT_YUVJ444P:
            return AV_PIX_FMT_YUV444P;
        case AV_PIX_FMT_YUVJ440P:
            return AV_PIX_FMT_YUV440P;
        default:
            return format;
        }
    }

    /// @brief FFmpeg state for a video reader
    struct FFmpegReader::ReaderState
    {
        AVFormatContext *formatContext = nullptr;
        // ---- video ----
        AVCodecParameters *videoCodecParameters = nullptr;
        const AVCodec *videoCodec = nullptr;
        std::string videoCodecName;
        int videoStreamIndex = -1;
        int videoWidth = 0;
        int videoHeight = 0;
        AVRational videoTimeBase{};
        int64_t videoNrOfFrames = 0;
        int64_t videoDuration = 0;
        double videoFrameRateHz = 0;
        AVCodecContext *videoCodecContext = nullptr;
        SwsContext *videoSwsContext = nullptr; // Pixel format conversion context
        // ---- audio ----
        AVCodecParameters *audioCodecParameters = nullptr;
        const AVCodec *audioCodec = nullptr;
        std::string audioCodecName;
        int audioStreamIndex = -1;
        AVRational audioTimeBase{};
        int64_t audioNrOfFrames = 0;
        int64_t audioDuration = 0;
        int64_t audioStartTime = 0;
        AVCodecContext *audioCodecContext = nullptr;
        SwrContext *audioSwrContext = nullptr;                    // Audio format conversion context
        AVChannelLayout audioOutChannelLayout;                    // Audio output channel layout
        int audioOutSampleRate = 0;                               // Audio output sample rate
        AVSampleFormat audioOutSampleFormat = AV_SAMPLE_FMT_NONE; // Audio output sample format
        uint8_t *audioOutData[2] = {nullptr, nullptr};            // Stereo audio conversion output sample buffer
        uint32_t audioOutDataNrOfSamples = 0;                     // Audio conversion output sample buffer size
        // ---- decoding ----
        AVFrame *frame = nullptr;
        AVPacket *packet = nullptr;
    };

    FFmpegReader::FFmpegReader()
        : m_state(std::make_shared<ReaderState>())
    {
    }

    FFmpegReader::~FFmpegReader()
    {
        close();
    }

    auto FFmpegReader::open(const std::string &filePath) -> void
    {
        REQUIRE(!filePath.empty(), std::runtime_error, "Empty file path passed");
        REQUIRE(m_state->formatContext == nullptr, std::runtime_error, "Reader already open. Call close() first");
        // Open the file using libavformat
        m_state->formatContext = avformat_alloc_context();
        REQUIRE(m_state->formatContext != nullptr, std::runtime_error, "Failed to create AVFormatContext");
        if (avformat_open_input(&m_state->formatContext, filePath.c_str(), nullptr, nullptr) != 0)
        {
            close();
            THROW(std::runtime_error, "Failed to open media file");
        }
        // Retrieve stream information
        if (avformat_find_stream_info(m_state->formatContext, nullptr) < 0)
        {
            close();
            THROW(std::runtime_error, "Failed to find stream info");
        }
        // Find the first valid video stream inside the file
        m_state->videoStreamIndex = -1;
        for (decltype(m_state->formatContext->nb_streams) i = 0; i < m_state->formatContext->nb_streams; i++)
        {
            auto stream = m_state->formatContext->streams[i];
            auto codecParams = stream->codecpar;
            if (codecParams != nullptr && codecParams->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                auto codec = avcodec_find_decoder(codecParams->codec_id);
                if (codec != nullptr)
                {
                    m_state->videoCodecParameters = codecParams;
                    m_state->videoCodec = codec;
                    m_state->videoCodecName = codec->long_name;
                    m_state->videoStreamIndex = static_cast<int>(i);
                    m_state->videoWidth = codecParams->width;
                    m_state->videoHeight = codecParams->height;
                    m_state->videoFrameRateHz = av_q2d(stream->r_frame_rate);
                    m_state->videoTimeBase = stream->time_base;
                    m_state->videoNrOfFrames = stream->nb_frames;
                    m_state->videoDuration = stream->duration;
                    break;
                }
            }
        }
        // Find the first audio stream inside the file
        m_state->audioStreamIndex = -1;
        for (decltype(m_state->formatContext->nb_streams) i = 0; i < m_state->formatContext->nb_streams; i++)
        {
            auto stream = m_state->formatContext->streams[i];
            auto codecParams = stream->codecpar;
            if (codecParams != nullptr && codecParams->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                auto codec = avcodec_find_decoder(codecParams->codec_id);
                if (codec != nullptr)
                {
                    m_state->audioCodecParameters = codecParams;
                    m_state->audioCodec = codec;
                    m_state->audioCodecName = codec->long_name;
                    m_state->audioStreamIndex = static_cast<int>(i);
                    m_state->audioTimeBase = stream->time_base;
                    m_state->audioNrOfFrames = stream->nb_frames;
                    m_state->audioDuration = stream->duration;
                    m_state->audioStartTime = stream->start_time;
                    REQUIRE(codecParams->ch_layout.nb_channels > 0, std::runtime_error, "Number of audio channels must be > 0");
                    av_channel_layout_copy(&m_state->audioOutChannelLayout, codecParams->ch_layout.nb_channels == 1 ? &monoLayout : &stereoLayout);
                    m_state->audioOutSampleRate = codecParams->sample_rate;
                    m_state->audioOutSampleFormat = AV_SAMPLE_FMT_S16P; // this is a planar format: L1 L2 ... R1 R2 ...
                    break;
                }
            }
        }
        // error out if we did not find any streams
        if (m_state->videoStreamIndex == -1 && m_state->audioStreamIndex == -1)
        {
            close();
            THROW(std::runtime_error, "Failed to find video or audio stream");
        }
        // Set up a codec context for the video decoder
        if (m_state->videoCodec != nullptr)
        {
            m_state->videoCodecContext = avcodec_alloc_context3(m_state->videoCodec);
            if (m_state->videoCodecContext == nullptr)
            {
                close();
                THROW(std::runtime_error, "Failed to create AVCodecContext for video");
            }
            if (avcodec_parameters_to_context(m_state->videoCodecContext, m_state->videoCodecParameters) < 0)
            {
                close();
                THROW(std::runtime_error, "Failed to initialize AVCodecContext for video");
            }
            if (avcodec_open2(m_state->videoCodecContext, m_state->videoCodec, nullptr) < 0)
            {
                close();
                THROW(std::runtime_error, "Failed to open video codec");
            }
        }
        // Set up a codec context for the audio decoder
        if (m_state->audioCodec != nullptr)
        {
            m_state->audioCodecContext = avcodec_alloc_context3(m_state->audioCodec);
            if (m_state->audioCodecContext == nullptr)
            {
                close();
                THROW(std::runtime_error, "Failed to create AVCodecContext for audio");
            }
            if (avcodec_parameters_to_context(m_state->audioCodecContext, m_state->audioCodecParameters) < 0)
            {
                close();
                THROW(std::runtime_error, "Failed to initialize AVCodecContext for audio");
            }
            if (avcodec_open2(m_state->audioCodecContext, m_state->audioCodec, nullptr) < 0)
            {
                close();
                THROW(std::runtime_error, "Failed to open audio codec");
            }
        }
        // Allocate frame and packet memory
        m_state->frame = av_frame_alloc();
        if (m_state->frame == nullptr)
        {
            close();
            THROW(std::runtime_error, "Failed to allocate frame");
        }
        m_state->packet = av_packet_alloc();
        if (m_state->packet == nullptr)
        {
            close();
            THROW(std::runtime_error, "Failed to allocate packet");
        }
        // generate media info
        if (m_state->videoStreamIndex >= 0 && m_state->videoCodec != nullptr)
        {
            m_info.fileType = static_cast<IO::FileType>(static_cast<uint8_t>(m_info.fileType) | IO::FileType::Video);
            m_info.videoCodecName = m_state->videoCodecName;
            m_info.videoStreamIndex = static_cast<uint32_t>(m_state->videoStreamIndex);
            m_info.videoWidth = static_cast<uint32_t>(m_state->videoWidth);
            m_info.videoHeight = static_cast<uint32_t>(m_state->videoHeight);
            m_info.videoNrOfFrames = static_cast<uint64_t>(m_state->videoNrOfFrames);
            m_info.videoDurationS = static_cast<double>(m_state->videoDuration) * static_cast<double>(m_state->videoTimeBase.num) / static_cast<double>(m_state->videoTimeBase.den);
            m_info.videoFrameRateHz = m_state->videoFrameRateHz;
            m_info.videoPixelFormat = Color::Format::XRGB8888;
            m_info.videoColorMapFormat = Color::Format::Unknown;
        }
        if (m_state->audioStreamIndex >= 0 && m_state->audioCodec != nullptr)
        {
            m_info.fileType = static_cast<IO::FileType>(static_cast<uint8_t>(m_info.fileType) | IO::FileType::Audio);
            m_info.audioNrOfFrames = static_cast<uint32_t>(m_state->audioNrOfFrames);
            m_info.audioNrOfSamples = static_cast<uint32_t>(m_state->audioDuration);
            m_info.audioDurationS = static_cast<double>(m_state->audioDuration) * static_cast<double>(m_state->audioTimeBase.num) / static_cast<double>(m_state->audioTimeBase.den);
            m_info.audioCodecName = m_state->audioCodecName;
            m_info.audioStreamIndex = static_cast<uint32_t>(m_state->audioStreamIndex);
            m_info.audioSampleRateHz = static_cast<uint32_t>(m_state->audioOutSampleRate);
            m_info.audioChannelFormat = m_state->audioOutChannelLayout.nb_channels == 1 ? Audio::ChannelFormat::Mono : Audio::ChannelFormat::Stereo;
            m_info.audioSampleFormat = Audio::SampleFormat::Signed16;
            m_info.audioOffsetS = static_cast<double>(m_state->audioStartTime) * static_cast<double>(m_state->audioTimeBase.num) / static_cast<double>(m_state->audioTimeBase.den);
        }
    }

    auto FFmpegReader::getInfo() const -> MediaInfo
    {
        return m_info;
    }

    auto FFmpegReader::readFrame() -> FrameData
    {
        bool isVideoFrame = false;
        bool isAudioFrame = false;
        while (true)
        {
            const auto readResult = av_read_frame(m_state->formatContext, m_state->packet);
            if (readResult == AVERROR_EOF)
            {
                // last packet. we need to keep receiving frames still queued in the decoder
                // the last packet will have ->data == NULL and ->size == 0 to mark it as a flush packet
            }
            else if (readResult < 0)
            {
                // some other read error
                av_packet_unref(m_state->packet);
                THROW(std::runtime_error, "Failed to read frame: " << readResult);
            }
            // check the stream index is audio or video
            if (m_state->packet->stream_index != m_state->videoStreamIndex && m_state->packet->stream_index != m_state->audioStreamIndex)
            {
                av_packet_unref(m_state->packet);
                continue;
            }
            // try to send packet to vaudio or video codec
            const bool isVideoPacket = m_state->packet->stream_index == m_state->videoStreamIndex;
            const auto sendResult = avcodec_send_packet(isVideoPacket ? m_state->videoCodecContext : m_state->audioCodecContext, m_state->packet);
            if (sendResult == AVERROR_EOF)
            {
                // file has ended. don't bail, but call avcodec_receive_frame to possibly get remaining frames
            }
            else if (sendResult == AVERROR(EAGAIN))
            {
                // we need to avcodec_receive_frame again before we can send another packet
            }
            else if (sendResult < 0)
            {
                av_packet_unref(m_state->packet);
                THROW(std::runtime_error, "Failed to send packet to " << (isVideoPacket ? "video" : "audio") << " codec: " << sendResult);
            }
            // try to decode frame
            const auto receiveResult = avcodec_receive_frame(isVideoPacket ? m_state->videoCodecContext : m_state->audioCodecContext, m_state->frame);
            if (receiveResult == AVERROR_EOF)
            {
                // end of frames encountered
                if (m_state->videoCodecContext)
                {
                    avcodec_flush_buffers(m_state->videoCodecContext);
                }
                if (m_state->audioCodecContext)
                {
                    avcodec_flush_buffers(m_state->audioCodecContext);
                }
                av_packet_unref(m_state->packet);
                return {};
            }
            else if (receiveResult == AVERROR(EAGAIN))
            {
                // decoder is busy. try again
                av_packet_unref(m_state->packet);
                continue;
            }
            else if (receiveResult < 0)
            {
                av_packet_unref(m_state->packet);
                THROW(std::runtime_error, "Failed to decode " << (isVideoPacket ? "video" : "audio") << " packet: " << receiveResult);
            }
            // here the frame has been successfully decoded
            isVideoFrame = m_state->packet->stream_index == m_state->videoStreamIndex;
            isAudioFrame = m_state->packet->stream_index == m_state->audioStreamIndex;
            av_packet_unref(m_state->packet);
            break;
        }
        // auto timeStamp = m_state->frame->pts; // timestamp when the frame should be shown
        if (isVideoFrame)
        {
            // set up sw scaler for pixel format conversion
            if (m_state->videoSwsContext == nullptr)
            {
                auto sourcePixelFormat = CorrectDeprecatedPixelFormat(m_state->videoCodecContext->pix_fmt);
                m_state->videoSwsContext = sws_getContext(m_state->videoWidth, m_state->videoHeight, sourcePixelFormat,
                                                          m_state->videoWidth, m_state->videoHeight, AV_PIX_FMT_0RGB32,
                                                          SWS_POINT, nullptr, nullptr, nullptr);

                REQUIRE(m_state->videoSwsContext != nullptr, std::runtime_error, "Failed to create video swscaler context");
            }
            // convert pixel format using sw scaler
            std::vector<Color::XRGB8888> frameData(m_state->videoWidth * m_state->videoHeight);
            uint8_t *const dst[4] = {reinterpret_cast<uint8_t *>(frameData.data()), nullptr, nullptr, nullptr};
            int const dstStride[4] = {m_state->videoWidth * static_cast<int>(sizeof(Color::XRGB8888)), 0, 0, 0};
            sws_scale(m_state->videoSwsContext, m_state->frame->data, m_state->frame->linesize, 0, m_state->frame->height, dst, dstStride);
            // release FFmpeg frame
            av_frame_unref(m_state->frame);
            return {IO::FrameType::Pixels, frameData};
        }
        else if (isAudioFrame)
        {
            // set up converter for audio format conversion
            const auto inSampleRate = m_state->audioCodecParameters->sample_rate;
            const auto inSampleFormat = static_cast<AVSampleFormat>(m_state->audioCodecParameters->format);
            if (m_state->audioSwrContext == nullptr)
            {
                auto allocResult = swr_alloc_set_opts2(&m_state->audioSwrContext, &m_state->audioOutChannelLayout, m_state->audioOutSampleFormat, inSampleRate, &m_state->audioCodecParameters->ch_layout, inSampleFormat, inSampleRate, 0, nullptr);
                REQUIRE(allocResult == 0 && m_state->audioSwrContext != nullptr, std::runtime_error, "Failed to allocate audio swresampler context: " << allocResult);
                auto initResult = swr_init(m_state->audioSwrContext);
                REQUIRE(initResult == 0, std::runtime_error, "Failed to init audio swresampler context: " << initResult);
            }
            // check if we need to reallocate audio conversion output buffers
            const auto nrOfSamplesNeeded = av_rescale_rnd(swr_get_delay(m_state->audioSwrContext, m_state->audioCodecParameters->sample_rate) + m_state->frame->nb_samples, inSampleRate, inSampleRate, AV_ROUND_UP);
            if (nrOfSamplesNeeded > m_state->audioOutDataNrOfSamples)
            {
                if (m_state->audioOutData[0] != nullptr)
                {
                    av_freep(&m_state->audioOutData[0]);
                    m_state->audioOutData[0] = nullptr;
                    m_state->audioOutData[1] = nullptr;
                    m_state->audioOutDataNrOfSamples = 0;
                }
                const auto allocateResult = av_samples_alloc(&m_state->audioOutData[0], nullptr, m_state->audioOutChannelLayout.nb_channels, nrOfSamplesNeeded, m_state->audioOutSampleFormat, 1);
                REQUIRE(allocateResult >= 0, std::runtime_error, "Failed to allocate audio conversion buffer: " << allocateResult);
                m_state->audioOutDataNrOfSamples = nrOfSamplesNeeded;
            }
            // convert audio format using sw resampler
            const auto nrOfSamplesConverted = swr_convert(m_state->audioSwrContext, &m_state->audioOutData[0], m_state->audioOutDataNrOfSamples, m_state->frame->extended_data, m_state->frame->nb_samples);
            REQUIRE(nrOfSamplesConverted >= 0, std::runtime_error, "Failed to convert audio data: " << nrOfSamplesConverted);
            // get size of a raw, combined, byte buffer needed to hold all sample data of all channels
            const auto convertedRawBufferSize = av_samples_get_buffer_size(nullptr, m_state->audioOutChannelLayout.nb_channels, nrOfSamplesConverted, m_state->audioOutSampleFormat, 1);
            REQUIRE(convertedRawBufferSize >= 0, std::runtime_error, "Failed to get number of audio samples output to buffer: " << convertedRawBufferSize);
            // copy converted audio to frame data
            std::vector<int16_t> frameData(convertedRawBufferSize / 2);
            auto dataPtr = reinterpret_cast<uint8_t *>(frameData.data());
            if (m_state->audioOutChannelLayout.nb_channels == 1)
            {
                std::memcpy(dataPtr, m_state->audioOutData[0], convertedRawBufferSize);
            }
            if (m_state->audioOutChannelLayout.nb_channels == 2)
            {
                std::memcpy(dataPtr, m_state->audioOutData[0], convertedRawBufferSize / 2);
                std::memcpy(dataPtr + convertedRawBufferSize / 2, m_state->audioOutData[1], convertedRawBufferSize / 2);
            }
            // release FFmpeg frame
            av_frame_unref(m_state->frame);
            return {IO::FrameType::Audio, frameData};
        }
        THROW(std::runtime_error, "Unexpected frame type");
    }

    auto FFmpegReader::close() -> void
    {
        if (m_state->packet)
        {
            av_packet_free(&m_state->packet);
            m_state->packet = nullptr;
        }
        if (m_state->frame)
        {
            av_frame_free(&m_state->frame);
            m_state->frame = nullptr;
        }
        if (m_state->videoSwsContext)
        {
            sws_freeContext(m_state->videoSwsContext);
            m_state->videoSwsContext = nullptr;
        }
        if (m_state->audioOutData[0])
        {
            av_freep(&m_state->audioOutData[0]);
            m_state->audioOutData[0] = nullptr;
            m_state->audioOutData[1] = nullptr;
            m_state->audioOutDataNrOfSamples = 0;
        }
        if (m_state->audioSwrContext)
        {
            swr_free(&m_state->audioSwrContext);
            m_state->audioSwrContext = nullptr;
        }
        if (m_state->videoCodecContext)
        {
            avcodec_free_context(&m_state->videoCodecContext);
            m_state->videoCodecContext = nullptr;
        }
        if (m_state->audioCodecContext)
        {
            avcodec_free_context(&m_state->audioCodecContext);
            m_state->audioCodecContext = nullptr;
        }
        if (m_state->formatContext)
        {
            avformat_close_input(&m_state->formatContext);
            avformat_free_context(m_state->formatContext);
            m_state->formatContext = nullptr;
        }
    }
}

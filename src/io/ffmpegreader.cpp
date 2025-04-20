#include "ffmpegreader.h"

extern "C"
{
#include <inttypes.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "exception.h"

namespace Video
{

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
        AVCodecParameters *codecParameters = nullptr;
        const AVCodec *codec = nullptr;
        std::string codecName;
        int videoStreamIndex = -1;
        int width = 0;
        int height = 0;
        float fps = 0;
        AVRational timeBase{};
        int64_t nrOfFrames = 0;
        int64_t duration = 0;
        AVCodecContext *codecContext = nullptr;
        AVFrame *frame = nullptr;
        AVPacket *packet = nullptr;
        SwsContext *swsContext = nullptr;
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
            THROW(std::runtime_error, "Failed to open video file");
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
                    m_state->codecParameters = codecParams;
                    m_state->codec = codec;
                    m_state->codecName = avcodec_get_name(codecParams->codec_id);
                    m_state->videoStreamIndex = static_cast<int>(i);
                    m_state->width = codecParams->width;
                    m_state->height = codecParams->height;
                    m_state->fps = av_q2d(stream->r_frame_rate);
                    m_state->timeBase = stream->time_base;
                    m_state->nrOfFrames = stream->nb_frames;
                    m_state->duration = stream->duration;
                    break;
                }
            }
        }
        if (m_state->videoStreamIndex == -1)
        {
            close();
            THROW(std::runtime_error, "Failed to find video stream");
        }
        // Set up a codec context for the decoder
        m_state->codecContext = avcodec_alloc_context3(m_state->codec);
        if (m_state->codecContext == nullptr)
        {
            close();
            THROW(std::runtime_error, "Failed to create AVCodecContext");
        }
        if (avcodec_parameters_to_context(m_state->codecContext, m_state->codecParameters) < 0)
        {
            close();
            THROW(std::runtime_error, "Failed to initialize AVCodecContext");
        }
        if (avcodec_open2(m_state->codecContext, m_state->codec, nullptr) < 0)
        {
            close();
            THROW(std::runtime_error, "Failed to open codec");
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
        // generate video info
        m_info.codecName = m_state->codecName;
        m_info.videoStreamIndex = static_cast<uint32_t>(m_state->videoStreamIndex);
        m_info.width = static_cast<uint32_t>(m_state->width);
        m_info.height = static_cast<uint32_t>(m_state->height);
        m_info.fps = m_state->fps;
        m_info.nrOfFrames = static_cast<uint64_t>(m_state->nrOfFrames);
        m_info.durationS = static_cast<float>(static_cast<double>(m_state->duration) * static_cast<double>(m_state->timeBase.num) / static_cast<double>(m_state->timeBase.den));
        m_info.pixelFormat = Color::Format::XRGB8888;
        m_info.colorMapFormat = Color::Format::Unknown;
    }

    auto FFmpegReader::getInfo() const -> VideoInfo
    {
        return m_info;
    }

    auto FFmpegReader::readFrame() -> std::vector<Color::XRGB8888>
    {
        while (true)
        {
            auto readResult = av_read_frame(m_state->formatContext, m_state->packet);
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
            // check if it is the correct stream index
            if (m_state->packet->stream_index != m_state->videoStreamIndex)
            {
                av_packet_unref(m_state->packet);
                continue;
            }
            // try to send packet to codec
            auto sendResult = avcodec_send_packet(m_state->codecContext, m_state->packet);
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
                THROW(std::runtime_error, "Failed to send packet to codec: " << sendResult);
            }
            // try to decode frame
            auto receiveResult = avcodec_receive_frame(m_state->codecContext, m_state->frame);
            if (receiveResult == AVERROR_EOF)
            {
                // end of frames encountered
                avcodec_flush_buffers(m_state->codecContext);
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
                THROW(std::runtime_error, "Failed to decode packet: " << receiveResult);
            }
            // here the frame has been successfully decoded
            av_packet_unref(m_state->packet);
            break;
        }
        // auto timeStamp = m_state->frame->pts; // timestamp when the frame should be shown
        // set up sw scaler for pixel format conversion
        if (m_state->swsContext == nullptr)
        {
            auto sourcePixelFormat = CorrectDeprecatedPixelFormat(m_state->codecContext->pix_fmt);
            m_state->swsContext = sws_getContext(m_state->width, m_state->height, sourcePixelFormat,
                                                 m_state->width, m_state->height, AV_PIX_FMT_0RGB32,
                                                 SWS_POINT, nullptr, nullptr, nullptr);
            if (m_state->swsContext == nullptr)
            {
                THROW(std::runtime_error, "Failed to create sw scaler");
            }
        }
        // convert pixel format using sw scaler
        std::vector<Color::XRGB8888> frame(m_state->width * m_state->height);
        uint8_t *const dst[4] = {reinterpret_cast<uint8_t *>(frame.data()), nullptr, nullptr, nullptr};
        int const dstStride[4] = {m_state->width * static_cast<int>(sizeof(Color::XRGB8888)), 0, 0, 0};
        sws_scale(m_state->swsContext, m_state->frame->data, m_state->frame->linesize, 0, m_state->frame->height, dst, dstStride);
        // release FFmpeg frame
        av_frame_unref(m_state->frame);
        return frame;
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
        if (m_state->swsContext)
        {
            sws_freeContext(m_state->swsContext);
            m_state->swsContext = nullptr;
        }
        if (m_state->codecContext)
        {
            avcodec_free_context(&m_state->codecContext);
            m_state->codecContext = nullptr;
        }
        if (m_state->formatContext)
        {
            avformat_close_input(&m_state->formatContext);
            avformat_free_context(m_state->formatContext);
            m_state->formatContext = nullptr;
        }
    }
}

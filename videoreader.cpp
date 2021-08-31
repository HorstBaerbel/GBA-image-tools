#include "videoreader.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <inttypes.h>
}

#include "exception.h"

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
struct VideoReader::ReaderState
{

    AVFormatContext *formatContext = nullptr;
    AVCodecParameters *codecParameters = nullptr;
    AVCodec *codec = nullptr;
    std::string codecName;
    int videoStreamIndex = -1;
    int width = 0;
    int height = 0;
    float fps = 0;
    AVRational timeBase{};
    AVCodecContext *codecContext = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *swsContext = nullptr;
};

VideoReader::VideoReader()
    : m_state(std::make_shared<ReaderState>())
{
}

VideoReader::~VideoReader()
{
    close();
}

void VideoReader::open(const std::string &filePath)
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
    // Find the first valid video stream inside the file
    m_state->videoStreamIndex = -1;
    for (decltype(m_state->formatContext->nb_streams) i = 0; i < m_state->formatContext->nb_streams; i++)
    {
        auto codecParams = m_state->formatContext->streams[i]->codecpar;
        auto codec = avcodec_find_decoder(codecParams->codec_id);
        if (codec != nullptr && codecParams != nullptr && codecParams->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_state->codecParameters = codecParams;
            m_state->codec = codec;
            m_state->codecName = avcodec_get_name(codecParams->codec_id);
            m_state->videoStreamIndex = static_cast<int>(i);
            m_state->width = codecParams->width;
            m_state->height = codecParams->height;
            m_state->fps = av_q2d(m_state->formatContext->streams[i]->r_frame_rate);
            m_state->timeBase = m_state->formatContext->streams[i]->time_base;
            break;
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
    // Set up sw scaler for pixel format conversion
    auto sourcePixelFormat = CorrectDeprecatedPixelFormat(m_state->codecContext->pix_fmt);
    m_state->swsContext = sws_getContext(m_state->width, m_state->height, sourcePixelFormat,
                                         m_state->width, m_state->height, AV_PIX_FMT_RGB24,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (m_state->swsContext == nullptr)
    {
        close();
        THROW(std::runtime_error, "Failed to create sw scaler");
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
}

VideoReader::VideoInfo VideoReader::getInfo() const
{
    REQUIRE(m_state->formatContext != nullptr, std::runtime_error, "Reader not open. Call open() first");
    return {m_state->codecName, static_cast<uint32_t>(m_state->videoStreamIndex), static_cast<uint32_t>(m_state->width), static_cast<uint32_t>(m_state->height), m_state->fps};
}

std::vector<uint8_t> VideoReader::readFrame() const
{
    while (av_read_frame(m_state->formatContext, m_state->packet) >= 0)
    {
        // check if it is the correct stream index
        if (m_state->packet->stream_index != m_state->videoStreamIndex)
        {
            av_packet_unref(m_state->packet);
            continue;
        }
        // send packet to codec
        REQUIRE(avcodec_send_packet(m_state->codecContext, m_state->packet) >= 0, std::runtime_error, "Failed to decode packet");
        // try to decode frame
        auto result = avcodec_receive_frame(m_state->codecContext, m_state->frame);
        if (result == AVERROR_EOF)
        {
            av_packet_unref(m_state->packet);
            return {};
        }
        else if (result == AVERROR(EAGAIN))
        {
            av_packet_unref(m_state->packet);
            continue;
        }
        else if (result < 0)
        {
            THROW(std::runtime_error, "Failed to decode packet");
        }
        av_packet_unref(m_state->packet);
        break;
    }
    //auto timeStamp = m_state->frame->pts; // timestamp when the frame should be shown
    // convert pixel format using sw scaler
    std::vector<uint8_t> frame(m_state->width, m_state->height * 3);
    uint8_t *const dst[4] = {frame.data(), nullptr, nullptr, nullptr};
    int const dstStride[4] = {m_state->width * 3, 0, 0, 0};
    sws_scale(m_state->swsContext, m_state->frame->data, m_state->frame->linesize, 0, m_state->frame->height, dst, dstStride);
    // release FFmpeg frame
    av_frame_unref(m_state->frame);
    return frame;
}

void VideoReader::close()
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
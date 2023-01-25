
#include <stdexcept>
#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
}

#include "./Frame.hpp"

namespace FFmpegSimple {

    Frame::Frame() {
        m_frame = av_frame_alloc();
        if (m_frame == nullptr)
            throw std::runtime_error("无法分配帧内存");
    }

    Frame::Frame(const uint8_t *data, enum AVPixelFormat format, int width, int height, int srcWidth, int srcHeight)
            : Frame() {
        m_frame->format = format;
        m_frame->width = width;
        m_frame->height = height;

        av_image_fill_arrays(m_frame->data, m_frame->linesize,
                             data,
                             format, srcWidth, srcHeight, 1);
    }

    Frame::Frame(const uint8_t *data, int64_t ch_layout, int samples, AVSampleFormat sample_fmt, int sample_rate)
            : Frame() {
        m_frame->channel_layout = ch_layout;
        m_frame->channels = av_get_channel_layout_nb_channels(ch_layout);
        m_frame->nb_samples = samples;
        m_frame->format = sample_fmt;
        m_frame->sample_rate = sample_rate;

        av_samples_fill_arrays(m_frame->data, m_frame->linesize,
                               data,
                               m_frame->channels, samples, sample_fmt, 1);
    }

    Frame::Frame(const std::vector<uint8_t *> &data, int64_t ch_layout, int samples, AVSampleFormat sample_fmt, int sample_rate)
            : Frame() {
        m_frame->channel_layout = ch_layout;
        m_frame->channels = av_get_channel_layout_nb_channels(ch_layout);
        m_frame->nb_samples = samples;
        m_frame->format = sample_fmt;
        m_frame->sample_rate = sample_rate;

        int line_size;
        int buf_size = av_samples_get_buffer_size(&line_size, m_frame->channels, samples, sample_fmt, 1);
        for (size_t ch = 0; ch < m_frame->channels; ++ch)
            m_frame->data[ch] = data.at(ch);
        *m_frame->linesize = line_size;
    }

    Frame::~Frame() {
        av_frame_free(&m_frame);
    }

}

#pragma once

#include <string>
#include <vector>

extern "C"
{
#include <libavformat/avformat.h>
}

namespace FFmpegSimple {

    class Frame {
    private:
        AVFrame *m_frame = nullptr;

        Frame();

    public:
        Frame(const uint8_t *data, enum AVPixelFormat format, int width, int height, int srcWidth, int srcHeight);

        Frame(const uint8_t *data, int64_t ch_layout, int samples, AVSampleFormat sample_fmt, int sample_rate);

        Frame(const std::vector<uint8_t *> &data, int64_t ch_layout, int samples, AVSampleFormat sample_fmt, int sample_rate);

        ~Frame();

        inline AVFrame *frame() { return m_frame; };
    };

}

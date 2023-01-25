#pragma once

extern "C"
{
#include <libavfilter/avfilter.h>
}

#include "./VideoDecode.hpp"
#include "./Encode.hpp"

namespace FFmpegSimple {

    class VideoConvert {
    private:
        AVFilterInOut *outputs;
        AVFilterInOut *inputs;
        AVFilterContext *buffersink_ctx;
        AVFilterContext *buffersrc_ctx;
        AVFilterGraph *filter_graph;

        AVFrame *frame;

    public:
        VideoConvert(int srcWidth, int srcHeight, enum AVPixelFormat srcFormat,
                     int dstWidth, int dstHeight, enum AVPixelFormat dstFormat,
                     Rotation rotation);

        VideoConvert(const VideoDecode &videoDecode,
                     int dstWidth, int dstHeight, enum AVPixelFormat dstFormat);

        VideoConvert(int srcWidth, int srcHeight, enum AVPixelFormat srcFormat,
                     const Encode &encode,
                     Rotation rotation);

        VideoConvert(const VideoDecode &videoDecode,
                     const Encode &encode);

        ~VideoConvert();

        const AVFrame *operator()(AVFrame *srcFrame);
    };

}

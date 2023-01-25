
#include <sstream>
#include <iostream>
#include <stdexcept>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
// #include <libavutil/imgutils.h>
}

#include "./VideoConvert.hpp"

namespace FFmpegSimple {

    VideoConvert::VideoConvert(int srcWidth, int srcHeight, enum AVPixelFormat srcFormat,
                               int dstWidth, int dstHeight, enum AVPixelFormat dstFormat,
                               Rotation rotation) {
        const AVFilter *buffersrc = avfilter_get_by_name("buffer");
        const AVFilter *buffersink = avfilter_get_by_name("buffersink");
        outputs = avfilter_inout_alloc();
        inputs = avfilter_inout_alloc();
        filter_graph = avfilter_graph_alloc();
        if (!outputs || !inputs || !filter_graph)
            throw std::runtime_error("Not enough memory");

        /* buffer video source: the decoded frames from the decoder will be inserted here. */
        std::stringstream avfilter_graph_create_filter_args;
        avfilter_graph_create_filter_args << "video_size=" << srcWidth << "x" << srcHeight << ":"
                                          << "pix_fmt=" << srcFormat << ":"
                                          << "time_base=1/1:"
                                          << "pixel_aspect=1/1";
        if (avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", avfilter_graph_create_filter_args.str().c_str(), nullptr, filter_graph) < 0)
            throw std::runtime_error("Cannot create buffer source");

        /* buffer video sink: to terminate the filter chain. */
        if (avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph) < 0)
            throw std::runtime_error("Cannot create buffer sink");

        AVPixelFormat pix_fmts[] = {dstFormat, AV_PIX_FMT_NONE};
        if (av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0)
            throw std::runtime_error("Cannot set output pixel format");

        /*
         * Set the endpoints for the filter graph. The filter_graph will
         * be linked to the graph described by filters_descr.
         */

        /*
         * The buffer source output must be connected to the input pad of
         * the first filter described by filters_descr; since the first
         * filter input label is not specified, it is set to "in" by
         * default.
         */
        outputs->name = av_strdup("in");
        outputs->filter_ctx = buffersrc_ctx;
        outputs->pad_idx = 0;
        outputs->next = NULL;

        /*
         * The buffer sink input must be connected to the output pad of
         * the last filter described by filters_descr; since the last
         * filter output label is not specified, it is set to "out" by
         * default.
         */
        inputs->name = av_strdup("out");
        inputs->filter_ctx = buffersink_ctx;
        inputs->pad_idx = 0;
        inputs->next = NULL;

        std::stringstream avfilter_graph_parse_ptr_args;
        switch (rotation) {
            case Rotation::Clock:
                avfilter_graph_parse_ptr_args << "transpose=clock,";
                break;
            case Rotation::Cclock:
                avfilter_graph_parse_ptr_args << "transpose=cclock,";
                break;
            case Rotation::Invert:
                avfilter_graph_parse_ptr_args << "hflip,vflip,";
                break;
        }
        avfilter_graph_parse_ptr_args << "scale=" << dstWidth << "x" << dstHeight;
        if (avfilter_graph_parse_ptr(filter_graph, avfilter_graph_parse_ptr_args.str().c_str(), &inputs, &outputs, NULL) < 0)
            throw std::runtime_error("avfilter_graph_parse_ptr failed");

        if (avfilter_graph_config(filter_graph, NULL) < 0)
            throw std::runtime_error("avfilter_graph_config failed");

        frame = av_frame_alloc();
        if (frame == nullptr)
            throw std::runtime_error("无法分配帧内存");
    }

    VideoConvert::VideoConvert(const VideoDecode &videoDecode, int dstWidth, int dstHeight, enum AVPixelFormat dstFormat) :
            VideoConvert(videoDecode.width(), videoDecode.height(), videoDecode.pix_fmt(),
                         dstWidth, dstHeight, dstFormat, videoDecode.rotation()) {}

    VideoConvert::VideoConvert(int srcWidth, int srcHeight, enum AVPixelFormat srcFormat, const Encode &encode, Rotation rotation) :
            VideoConvert(srcWidth, srcHeight, srcFormat,
                         encode.width(), encode.height(), encode.pix_fmt(), rotation) {}

    VideoConvert::VideoConvert(const VideoDecode &videoDecode, const Encode &encode) :
            VideoConvert(videoDecode.width(), videoDecode.height(), videoDecode.pix_fmt(),
                         encode.width(), encode.height(), encode.pix_fmt(), videoDecode.rotation()) {}

    VideoConvert::~VideoConvert() {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        avfilter_graph_free(&filter_graph);
        av_frame_free(&frame);
    }

    const AVFrame *VideoConvert::operator()(AVFrame *srcFrame) {
        /* push the decoded frame into the filtergraph */
        if (av_buffersrc_add_frame_flags(buffersrc_ctx, srcFrame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
            throw std::runtime_error("Error while feeding the filtergraph");

        /* pull filtered frames from the filtergraph */
        int ret = av_buffersink_get_frame(buffersink_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0)
            throw std::runtime_error("av_buffersink_get_frame failed");
        return frame;
    }

}

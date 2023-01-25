#pragma once

#include <string>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace FFmpegSimple {

    static const enum AVPixelFormat CvFormat = AV_PIX_FMT_BGRA;

    class Encode {
    private:
        AVFormatContext *fmt_ctx = nullptr;
        AVStream *videoStream = nullptr;
        AVCodecContext *videoCodec_ctx = nullptr;
        AVPacket *videoPacket = nullptr;
        int videoPts = 0;
        AVStream *audioStream = nullptr;
        AVCodecContext *audioCodec_ctx = nullptr;
        AVPacket *audioPacket = nullptr;
        int audioPts = 0;

        void writeVideoPacket();

        void sendVideoFrame(const AVFrame *frame);

        void writeAudioPacket();

        void sendAudioFrame(const AVFrame *frame);

    public:
        Encode(const std::string &filePath, int width, int height, int fps, int sample_rate);

        ~Encode();

        void encodeVideo(const AVFrame *frame);

        void flushVideo();

        void encodeAudio(const AVFrame *frame);

        void flushAudio();

        inline auto width() const { return videoCodec_ctx->width; }

        inline auto height() const { return videoCodec_ctx->height; }

        inline auto pix_fmt() const { return videoCodec_ctx->pix_fmt; }

        inline auto sample_rate() const { return audioCodec_ctx->sample_rate; }

        inline auto channels() const { return audioCodec_ctx->channels; }

        inline auto channel_layout() const { return audioCodec_ctx->channel_layout; }

        inline auto sample_fmt() const { return audioCodec_ctx->sample_fmt; }

        inline auto frame_size() const { return audioCodec_ctx->frame_size; }
    };

}

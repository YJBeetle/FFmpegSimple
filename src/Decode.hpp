#pragma once

#include <string>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

    class Decode {
    protected:
        AVFormatContext *fmt_ctx = nullptr; //源文件格式信息
        int stream_idx = -1; // 流id
        AVStream *stream = nullptr; // 流
        AVCodecContext *dec_ctx = nullptr; // 解码器ctx

        // 解码
        AVPacket *currentPkt = nullptr;
        AVFrame *currentFrame = nullptr;

        void getNewPkt();

        void getNewFrame();

    public:

        Decode(const std::string &filePath, enum AVMediaType type);

        virtual ~Decode();

        void readFrame(int64_t timestamp);

        void readFrame();

        inline AVFrame *frame() const { return currentFrame; }

        inline int64_t duration_ts() const { return stream->duration; }

        inline double duration() const { return stream->duration * av_q2d(stream->time_base); }

    };

}

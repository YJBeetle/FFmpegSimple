
#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

#include "./Encode.hpp"

namespace Ffmpeg {

    Encode::Encode(const std::string &filePath, int width, int height, int fps, int sample_rate) {
#ifdef VERBOSE
        std::cout << "Encode file: " << filePath << "\n";
#endif
        avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, filePath.c_str());
        if (fmt_ctx == nullptr)
            throw std::runtime_error("avformat_alloc_output_context2 fail");

        if (avio_open(&fmt_ctx->pb, filePath.c_str(), AVIO_FLAG_WRITE) < 0)
            throw std::runtime_error("avio_open fail");

        // Video
        videoStream = avformat_new_stream(fmt_ctx, nullptr);
        if (videoStream == nullptr)
            throw std::runtime_error("new stream fail");

        const AVCodec *videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (videoCodec == nullptr)
            throw std::runtime_error("找不到流的编码器");
        videoCodec_ctx = avcodec_alloc_context3(videoCodec);
        if (videoCodec_ctx == nullptr)
            throw std::runtime_error("无法分配编码器context");
        videoStream->id = fmt_ctx->nb_streams - 1;
        videoStream->time_base = videoCodec_ctx->time_base = AVRational{.num=1, .den=fps};
        videoStream->avg_frame_rate = videoCodec_ctx->framerate = AVRational{.num=fps, .den=1}; // CFR
//        videoCodec_ctx->ticks_per_frame
        videoCodec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        videoCodec_ctx->width = width;
        videoCodec_ctx->height = height;
        videoCodec_ctx->bit_rate = 5 * 1000 * 1000; // 5000 Mb/s
        videoCodec_ctx->gop_size = 128; // 关键帧间隔
        videoCodec_ctx->has_b_frames = 1; // B帧
        videoCodec_ctx->max_b_frames = 128;
        videoCodec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        videoCodec_ctx->me_range = 16;
        videoCodec_ctx->max_qdiff = 4;
        videoCodec_ctx->qcompress = 0.6;
        videoCodec_ctx->qmin = 10;
        videoCodec_ctx->qmax = 51;
        av_opt_set(videoCodec_ctx->priv_data, "preset", "fast", 0); // or slow
        av_opt_set(videoCodec_ctx->priv_data, "tune", "zerolatency", 0);
        av_opt_set(videoCodec_ctx->priv_data, "profile", "main", 0);

        if (avcodec_open2(videoCodec_ctx, videoCodec, nullptr) < 0)
            throw std::runtime_error("无法打开编码器");

        if (avcodec_parameters_from_context(videoStream->codecpar, videoCodec_ctx) < 0)
            throw std::runtime_error("无法将编解码器参数复制从context");

        videoPacket = av_packet_alloc();
        if (videoPacket == nullptr)
            throw std::runtime_error("无法分配包内存");

        // Audio
        audioStream = avformat_new_stream(fmt_ctx, nullptr);
        if (audioStream == nullptr)
            throw std::runtime_error("new stream fail");

        const AVCodec *audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (audioCodec == nullptr)
            throw std::runtime_error("找不到流的编码器");
        audioCodec_ctx = avcodec_alloc_context3(audioCodec);
        if (audioCodec_ctx == nullptr)
            throw std::runtime_error("无法分配编码器context");
        audioStream->id = fmt_ctx->nb_streams - 1;
        audioStream->time_base = audioCodec_ctx->time_base = AVRational{.num=1, .den=sample_rate};
        audioCodec_ctx->sample_rate = sample_rate;
        audioCodec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
        audioCodec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
        audioCodec_ctx->channels = av_get_channel_layout_nb_channels(audioCodec_ctx->channel_layout);
        audioCodec_ctx->bit_rate = 256 * 1000; // 256 kb/s

        if (avcodec_open2(audioCodec_ctx, audioCodec, nullptr) < 0)
            throw std::runtime_error("无法打开编码器");

        if (avcodec_parameters_from_context(audioStream->codecpar, audioCodec_ctx) < 0)
            throw std::runtime_error("无法将编解码器参数复制从context");

        audioPacket = av_packet_alloc();
        if (audioPacket == nullptr)
            throw std::runtime_error("无法分配包内存");

#ifdef VERBOSE
        av_dump_format(fmt_ctx, 0, filePath.c_str(), 1);
#endif

        if (avformat_write_header(fmt_ctx, nullptr) < 0)
            throw std::runtime_error("cann't write the file head");
    }

    Encode::~Encode() {
        av_write_trailer(fmt_ctx);
        avio_close(fmt_ctx->pb);
        avformat_free_context(fmt_ctx);

        av_packet_free(&videoPacket);
        av_packet_free(&audioPacket);
    }

    void Encode::writeVideoPacket() {
#ifdef VERBOSE
        std::cout << "writeVideoPacket:"
                  << " pkt->size:" << videoPacket->size
                  << " pts:" << videoPacket->pts
                  << " dts:" << videoPacket->dts
                  << " duration:" << videoPacket->duration
                  << "\n";
#endif
        av_packet_rescale_ts(videoPacket, videoCodec_ctx->time_base, videoStream->time_base);
        videoPacket->stream_index = videoStream->index;
        if (av_interleaved_write_frame(fmt_ctx, videoPacket) < 0)
            throw std::runtime_error("Error while writing video frame");
    }

    void Encode::sendVideoFrame(const AVFrame *frame) {
#ifdef VERBOSE
        if (frame)
            std::cout << "sendVideoFrame:"
                      << " pts:" << frame->pts
                      << " width:" << frame->width
                      << " height:" << frame->height
                      << "\n";
#endif
        if (avcodec_send_frame(videoCodec_ctx, frame) < 0)
            throw std::runtime_error("Error sending a frame for encoding from encode video");
        switch (avcodec_receive_packet(videoCodec_ctx, videoPacket)) {
            case 0:
                writeVideoPacket();
                break;
            case AVERROR(EINVAL):
                throw std::runtime_error("Error while getting video packet");
            case AVERROR_EOF:
                throw std::runtime_error("End of stream met while adding a frame");
            case AVERROR(EAGAIN):
                break;
        }
        av_packet_unref(videoPacket);
    }

    void Encode::writeAudioPacket() {
#ifdef VERBOSE
        std::cout << "writeAudioPacket:"
                  << " pkt->size:" << audioPacket->size
                  << " pts:" << audioPacket->pts
                  << " dts:" << audioPacket->dts
                  << " duration:" << audioPacket->duration
                  << "\n";
#endif
        av_packet_rescale_ts(audioPacket, audioCodec_ctx->time_base, audioStream->time_base);
        audioPacket->stream_index = audioStream->index;
        if (av_interleaved_write_frame(fmt_ctx, audioPacket) < 0)
            throw std::runtime_error("Error while writing audio frame");
    }

    void Encode::sendAudioFrame(const AVFrame *frame) {
#ifdef VERBOSE
        if (frame)
            std::cout << "sendAudioFrame:"
                      << " pts:" << frame->pts
                      << "\n";
#endif
        if (avcodec_send_frame(audioCodec_ctx, frame) < 0)
            throw std::runtime_error("Error sending a frame for encoding from encode audio");
        switch (avcodec_receive_packet(audioCodec_ctx, audioPacket)) {
            case 0:
                writeAudioPacket();
                break;
            case AVERROR(EINVAL):
                throw std::runtime_error("Error while getting audio packet");
            case AVERROR_EOF:
                throw std::runtime_error("End of stream met while adding a frame");
            case AVERROR(EAGAIN):
                break;
        }
        av_packet_unref(audioPacket);
    }

    void Encode::encodeVideo(const AVFrame *frame) {
        ((AVFrame *) frame)->pts = videoPts++;
        sendVideoFrame(frame);
    }

    void Encode::flushVideo() {
        if (avcodec_send_frame(videoCodec_ctx, nullptr) < 0)
            throw std::runtime_error("Error sending a frame for encoding from flush video");

        int ret = 0;
        do {
            switch (ret = avcodec_receive_packet(videoCodec_ctx, videoPacket)) {
                case 0:
                    writeVideoPacket();
                    break;
                case AVERROR(EINVAL) :
                    throw std::runtime_error("Error while getting video packet");
            }
            av_packet_unref(videoPacket);
        } while (ret != AVERROR_EOF);
    }

    void Encode::encodeAudio(const AVFrame *frame) {
        ((AVFrame *) frame)->pts = audioPts;
        audioPts += frame->nb_samples;
        sendAudioFrame(frame);
    }

    void Encode::flushAudio() {
        if (avcodec_send_frame(audioCodec_ctx, nullptr) < 0)
            throw std::runtime_error("Error sending a frame for encoding from flush audio");

        int ret = 0;
        do {
            switch (ret = avcodec_receive_packet(audioCodec_ctx, audioPacket)) {
                case 0:
                    writeAudioPacket();
                    break;
                case AVERROR(EINVAL) :
                    throw std::runtime_error("Error while getting audio packet");
            }
            av_packet_unref(audioPacket);
        } while (ret != AVERROR_EOF);
    }

}


#include <iostream>
#include "./Decode.hpp"

namespace FFmpegSimple {

    Decode::Decode(const std::string &filePath, enum AVMediaType type) {
        // 打开文件读取格式
        if (avformat_open_input(&fmt_ctx, filePath.c_str(), nullptr, nullptr) < 0)
            throw std::runtime_error("无法打开源文件");

#ifdef VERBOSE
        // 显示输入文件信息
        // av_dump_format(fmt_ctx, 0, filePath.c_str(), 0);
#endif

        // 检索流信息
        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0)
            throw std::runtime_error("找不到流信息");

        // 打开流
        stream_idx = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
        if (stream_idx < 0)
            throw std::runtime_error("找不到流");
        stream = fmt_ctx->streams[stream_idx]; // 流

        // 寻找视频流的解码器
        const AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id); // 流的解码器
        if (dec == nullptr)
            throw std::runtime_error("找不到流的解码器");

        // 为解码器分配context
        dec_ctx = avcodec_alloc_context3(dec);
        if (dec_ctx == nullptr)
            throw std::runtime_error("无法分配编解码器context");

        // 将编解码器参数从输入流复制到输出编解码器context
        if (avcodec_parameters_to_context(dec_ctx, stream->codecpar) < 0)
            throw std::runtime_error("无法将编解码器参数复制到context");

        // 启动解码器
        if (avcodec_open2(dec_ctx, dec, nullptr) < 0)
            throw std::runtime_error("无法打开编解码器");

        currentPkt = av_packet_alloc();
        if (currentPkt == nullptr)
            throw std::runtime_error("无法分配包内存");
        currentFrame = av_frame_alloc();
        if (currentFrame == nullptr)
            throw std::runtime_error("无法分配帧内存");
    }

    Decode::~Decode() {
        av_packet_free(&currentPkt);
        av_frame_free(&currentFrame);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
    }

    void Decode::getNewPkt() {
        av_packet_unref(currentPkt); // 貌似多unref几次没啥问题

        restartBecauseStreamIdIsIncorrect:
        switch (av_read_frame(fmt_ctx, currentPkt)) {
            case 0 :
                if (currentPkt->stream_index != stream_idx) { // 不是当前流
                    av_packet_unref(currentPkt);
                    goto restartBecauseStreamIdIsIncorrect;
                }
                // 解码
                if (avcodec_send_packet(dec_ctx, currentPkt) < 0)
                    throw std::runtime_error("发送数据包进行解码时出错");
#ifdef VERBOSE
//                std::cout << "getNewPkt:"
//                          << " pkt->size:" << currentPkt->size
//                          << " pts:" << currentPkt->pts
//                          << " dts:" << currentPkt->dts
//                          << " duration:" << currentPkt->duration
//                          << "\n";
#endif
                break;
            case AVERROR_EOF:
                // flush
                if (avcodec_send_packet(dec_ctx, nullptr) < 0)
                    throw std::runtime_error("发送数据包进行解码时出错");
                break;
            case AVERROR(EAGAIN):
            default:
                throw std::runtime_error("无法读取新包");
        }
    }

    void Decode::getNewFrame() {
        restartBecauseNeedNewPkt:
        switch (avcodec_receive_frame(dec_ctx, currentFrame)) {
            case 0:
                // 正常获取帧 返回
#ifdef VERBOSE
                // 输出帧信息
                std::cout << "getNewFrame:"
                          << " pts:" << currentFrame->pts
                          << " pkt_dts:" << currentFrame->pkt_dts;
                switch (dec_ctx->codec_type) {
                    case AVMEDIA_TYPE_VIDEO :
                        std::cout << " coded_n:" << currentFrame->coded_picture_number
                                  << " rescalePts:" << av_rescale_q(currentFrame->pts, stream->time_base, av_inv_q(stream->avg_frame_rate));
                        break;
                    case AVMEDIA_TYPE_AUDIO :
                        std::cout << " nb_samples:" << currentFrame->nb_samples;
                                //   << " ts2timestr:" << av_ts2timestr(currentFrame->pts, &dec_ctx->time_base);
                        break;
                    default:
                        break;
                }
                std::cout << "\n";

//            int data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt); // 音频
//            if (data_size < 0)
//                throw std::runtime_error("无法计算数据大小");
//            for (int i = 0; i < currentFrame->nb_samples; i++)
//                for (int ch = 0; ch < dec_ctx->channels; ch++) {
//                    // fwrite(frame->data[ch] + data_size * i, 1, data_size, audio_dst_file);
//                }
#endif
                return;
            case AVERROR_EOF:
                throw std::runtime_error("已经到达文件末尾");
            case AVERROR(EAGAIN):
                getNewPkt();
                goto restartBecauseNeedNewPkt;
            default:
                throw std::runtime_error("解码时出错");
        }
    }

    void Decode::readFrame(int64_t frameNum) {
        if (av_rescale_q(currentFrame->pts, stream->time_base, av_inv_q(stream->avg_frame_rate)) == frameNum - 1)
            getNewFrame();
        else {
            av_seek_frame(fmt_ctx, stream_idx, frameNum, AVSEEK_FLAG_BACKWARD);
            do
                getNewFrame();
            while (av_rescale_q(currentFrame->pts, stream->time_base, av_inv_q(stream->avg_frame_rate)) != frameNum);
        }
    }

    void Decode::readFrame() {
        getNewFrame();
    }

}

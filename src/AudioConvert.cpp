
#include <stdexcept>

extern "C"
{
#include <libavutil/opt.h>
}

#include "./AudioConvert.hpp"

namespace Ffmpeg {

    AudioConvert::AudioConvert(int64_t src_ch_layout_, enum AVSampleFormat src_sample_fmt_, int src_sample_rate_,
                               int64_t dst_ch_layout_, enum AVSampleFormat dst_sample_fmt_, int dst_sample_rate_) :
            src_ch_layout(src_ch_layout_), src_channels(av_get_channel_layout_nb_channels(src_ch_layout)), src_sample_fmt(src_sample_fmt_), src_sample_rate(src_sample_rate_),
            dst_ch_layout(dst_ch_layout_), dst_channels(av_get_channel_layout_nb_channels(dst_ch_layout)), dst_sample_fmt(dst_sample_fmt_), dst_sample_rate(dst_sample_rate_) {
        // 音频重采样
        convert_ctx = swr_alloc_set_opts(nullptr,
                                         dst_ch_layout, dst_sample_fmt, dst_sample_rate,
                                         src_ch_layout, src_sample_fmt, src_sample_rate,
                                         0, NULL);
        if (convert_ctx == nullptr)
            throw std::runtime_error("无法设置重采样ctx");

//        convert_ctx = swr_alloc();
//        if (convert_ctx == nullptr)
//            throw std::runtime_error("无法设置重采样ctx");
//        av_opt_set_int(convert_ctx, "in_channel_layout", src_ch_layout, 0);
//        av_opt_set_int(convert_ctx, "in_sample_rate", src_sample_rate, 0);
//        av_opt_set_sample_fmt(convert_ctx, "in_sample_fmt", src_sample_fmt, 0);
//        av_opt_set_int(convert_ctx, "out_channel_layout", dst_ch_layout, 0);
//        av_opt_set_int(convert_ctx, "out_sample_rate", dst_sample_rate, 0);
//        av_opt_set_sample_fmt(convert_ctx, "out_sample_fmt", dst_sample_fmt, 0);

        if (swr_init(convert_ctx) < 0)
            throw std::runtime_error("无法初始化重采样ctx");
    }

    AudioConvert::AudioConvert(const AudioDecode &audioDecode, int64_t dst_ch_layout, enum AVSampleFormat dst_sample_fmt, int dst_sample_rate) :
            AudioConvert(audioDecode.channel_layout(), audioDecode.sample_fmt(), audioDecode.sample_rate(),
                         dst_ch_layout, dst_sample_fmt, dst_sample_rate) {}

    AudioConvert::AudioConvert(int64_t src_ch_layout, enum AVSampleFormat src_sample_fmt, int src_sample_rate, const Encode &encode) :
            AudioConvert(src_ch_layout, src_sample_fmt, src_sample_rate,
                         encode.channel_layout(), encode.sample_fmt(), encode.sample_rate()) {}

    AudioConvert::AudioConvert(const AudioDecode &audioDecode, const Encode &encode) :
            AudioConvert(audioDecode.channel_layout(), audioDecode.sample_fmt(), audioDecode.sample_rate(),
                         encode.channel_layout(), encode.sample_fmt(), encode.sample_rate()) {}

    AudioConvert::~AudioConvert() {
        swr_free(&convert_ctx);
    }

    std::pair<std::vector<std::vector<uint8_t>>, int> AudioConvert::operator()(const std::vector<std::vector<uint8_t>> &srcData, int srcSamples) {
        int dstSamples = av_rescale_rnd(swr_get_delay(convert_ctx, src_sample_rate) + srcSamples,
                                        dst_sample_rate, src_sample_rate, AV_ROUND_UP);
        const int dstBytesPerSample = av_get_bytes_per_sample(dst_sample_fmt);

        std::vector<std::vector<uint8_t>> dstData;
        dstData.reserve(dst_channels);
        for (int channel = 0; channel < dst_channels; ++channel)
            dstData.emplace_back(dstSamples * dstBytesPerSample);

        std::vector<const uint8_t *> pSrcData(src_channels);
        for (int channel = 0; channel < src_channels; ++channel)
            pSrcData[channel] = srcData.at(channel).data();

        std::vector<uint8_t *> pDstData(dst_channels);
        for (int channel = 0; channel < dst_channels; ++channel)
            pDstData[channel] = dstData[channel].data();

        dstSamples = swr_convert(convert_ctx, pDstData.data(), dstSamples, pSrcData.data(), srcSamples);
        if (dstSamples < 0)
            throw std::runtime_error("Error while converting");

        return {dstData, dstSamples};
    }

}

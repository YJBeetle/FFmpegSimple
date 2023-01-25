#pragma once

#include <vector>

extern "C"
{
#include <libswresample/swresample.h>
}

#include "./AudioDecode.hpp"
#include "./Encode.hpp"

namespace Ffmpeg {

    class AudioConvert {
    private:
        int64_t src_ch_layout;
        int src_channels;
        enum AVSampleFormat src_sample_fmt;
        int src_sample_rate;
        int64_t dst_ch_layout;
        int dst_channels;
        enum AVSampleFormat dst_sample_fmt;
        int dst_sample_rate;

        SwrContext *convert_ctx = nullptr;
    public:
        // eg: AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLTP, 44100 // 44100双声道32位浮点
        AudioConvert(int64_t src_ch_layout_, enum AVSampleFormat src_sample_fmt_, int src_sample_rate_,
                     int64_t dst_ch_layout_, enum AVSampleFormat dst_sample_fmt_, int dst_sample_rate_);

        AudioConvert(const AudioDecode &audioDecode,
                     int64_t dst_ch_layout, enum AVSampleFormat dst_sample_fmt, int dst_sample_rate);

        AudioConvert(int64_t src_ch_layout, enum AVSampleFormat src_sample_fmt, int src_sample_rate,
                     const Encode &encode);

        AudioConvert(const AudioDecode &audioDecode,
                     const Encode &encode);

        ~AudioConvert();

        std::pair<std::vector<std::vector<uint8_t>>, int> operator()(const std::vector<std::vector<uint8_t>> &srcData, int srcSamples);
    };

}

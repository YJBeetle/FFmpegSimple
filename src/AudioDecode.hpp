#pragma once

#include "./Decode.hpp"

namespace Ffmpeg {

    class AudioDecode : public Decode {
    private:

    public:
        AudioDecode(const std::string &filePath);

        ~AudioDecode() {}

        inline auto sample_rate() const { return dec_ctx->sample_rate; }

        inline auto channels() const { return dec_ctx->channels; }

        inline auto channel_layout() const { return dec_ctx->channel_layout; }

        inline auto sample_fmt() const { return dec_ctx->sample_fmt; }

        inline auto frame_size() const { return dec_ctx->frame_size; }

    };

}

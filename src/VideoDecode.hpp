#pragma once

#include "./Decode.hpp"

namespace Ffmpeg {

    enum class Rotation {
        Normal,
        Clock,
        Cclock,
        Invert,
    };

    class VideoDecode : public Decode {
    private:

        Rotation m_rotation = Rotation::Normal;

    public:

        VideoDecode(const std::string &filePath);

        ~VideoDecode() {}

        inline auto width() const { return dec_ctx->width; }

        inline auto height() const { return dec_ctx->height; }

        inline auto pix_fmt() const { return dec_ctx->pix_fmt; }

        inline auto frames() const { return stream->nb_frames; }

        inline auto rotation() const { return m_rotation; }
    };

}

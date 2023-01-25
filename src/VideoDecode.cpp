#include <cstring>

extern "C"
{
#include <libavutil/display.h>
}

#include "./VideoDecode.hpp"

namespace Ffmpeg {

    VideoDecode::VideoDecode(const std::string &filePath) : Decode(filePath, AVMEDIA_TYPE_VIDEO) {
        // 确定视频旋转
        if (stream->side_data != nullptr) {
            auto displayMatrix = (int32_t *) av_stream_get_side_data(stream, AV_PKT_DATA_DISPLAYMATRIX, nullptr);
            auto r = av_display_rotation_get(displayMatrix);
            if (r == -90.)
                m_rotation = Rotation::Clock;
            else if (r == 90.)
                m_rotation = Rotation::Cclock;
            else if (r == 180. || r == -180.)
                m_rotation = Rotation::Invert;
        }
    }

}

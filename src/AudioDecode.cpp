
#include "./AudioDecode.hpp"

namespace FFmpegSimple {

    AudioDecode::AudioDecode(const std::string &filePath) : Decode(filePath, AVMEDIA_TYPE_AUDIO) {
    }

}

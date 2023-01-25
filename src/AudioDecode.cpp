
#include "./AudioDecode.hpp"

namespace Ffmpeg {

    AudioDecode::AudioDecode(const std::string &filePath) : Decode(filePath, AVMEDIA_TYPE_AUDIO) {
    }

}

#include "preprocessor.hpp"
#include <iostream>

// libavformat
extern "C" {
#include <libavformat/avformat.h>
}

void Preprocessor::InitLibAV() {
    if (!avformat_init) {
        std::cout << "Initializing libavformat..." << std::endl;
        av_log_set_level(AV_LOG_ERROR);
        avformat_init = true;
    }
}

Preprocessor::Preprocessor (std::filesystem::path) {
    InitLibAV();
}
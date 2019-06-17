// libavformat
extern "C" {
#include <libavformat/avformat.h>
}

#include <filesystem.hpp>
#include "types.hpp"

std::optional<SoundDescriptor> GetSoundDescriptor (const std::filesystem::path& path)
{
    CPP_AVFormatContext ps;
    AVFormatContext *_ps = ps.get();
    avformat_open_input (ps.get_ptr(), path.string().c_str(), NULL, NULL);
    if (!ps) return std::nullopt;

    avformat_find_stream_info (_ps, NULL);
    int64_t duration = _ps->duration;
    if (duration <= 0) return std::nullopt;
    if (_ps->nb_streams != 1) return std::nullopt;

    return SoundDescriptor (
        Duration (static_cast<double>(duration)/AV_TIME_BASE),
        Samplerate (_ps->streams[0]->codecpar->sample_rate)
    );
}

std::filesystem::path strip_root (const std::filesystem::path& p) {
    const std::filesystem::path& parent_path = p.parent_path();
    if (parent_path.empty() || parent_path.string() == "/")
        return std::filesystem::path();
    else
        return strip_root(parent_path) / p.filename();
}
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <filesystem.hpp>
#include "types.hpp"

std::optional<SoundProperties> GetSoundProperties (const std::filesystem::path& path) {
    CPP_AVFormatContext ps;
    AVFormatContext *_ps = ps.get();
    avformat_open_input (ps.get_ptr(), path.string().c_str(), NULL, NULL);
    if (!ps) return std::nullopt;

    avformat_find_stream_info (_ps, NULL);
    int64_t duration = _ps->duration;
    if (duration <= 0) return std::nullopt;
    if (_ps->nb_streams != 1) return std::nullopt;

    return SoundProperties (
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

int getNumberOfDirectories (std::filesystem::path path) {
    return std::count_if (std::filesystem::directory_iterator(path),
        std::filesystem::directory_iterator(),
        bind (static_cast<bool(*)(const std::filesystem::path&)> (std::filesystem::is_directory),
            bind (&std::filesystem::directory_entry::path, _1)));
}

bool cmp_ifspath (const std::filesystem::path& first, const std::filesystem::path& second) {
    return boost::algorithm::ilexicographical_compare(first.c_str(), second.c_str());
}

bool cmp_sfi (const SoundFileInfo& first, const SoundFileInfo& second) {
    return boost::algorithm::ilexicographical_compare(first.getPath().c_str(), second.getPath().c_str());
}
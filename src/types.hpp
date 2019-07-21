#ifndef CP_TYPES_HPP
#define CP_TYPES_HPP

#include <named_type.hpp>
#include <filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <tuple>
#include <list>
#include <map>
#include <string>

// libavformat
extern "C" {
#include <libavformat/avformat.h>
}

// Sound Properties

using Duration = fluent::NamedType<double, struct DurationParameter, fluent::FunctionCallable, fluent::Printable>;
using Samplerate = fluent::NamedType<float, struct SammplerateParameter, fluent::FunctionCallable, fluent::Printable>;

class SoundProperties {
    public:
        explicit SoundProperties (Duration d, Samplerate s) : duration_(d), samplerate_(s) {}

        Duration getDuration() const { return duration_; }
        Samplerate getSamplerate() const { return samplerate_; }
    private:
        Duration duration_;
        Samplerate samplerate_;
};

class SoundFileInfo : public SoundProperties {
    public:
        explicit SoundFileInfo (std::filesystem::path p, Duration d, Samplerate s) : path_(p), SoundProperties(d, s) {}
        explicit SoundFileInfo (std::filesystem::path p, SoundProperties sp) : path_(p), SoundProperties(sp) {}

        const bool operator < (const SoundFileInfo &other) const {
            return boost::algorithm::ilexicographical_compare (path_.c_str(), other.getPath().c_str());
        }

        std::filesystem::path getPath() const { return path_; }
    private:
        std::filesystem::path path_;
};

using SoundName = fluent::NamedType<std::string, struct SoundNameParameter, fluent::Comparable, fluent::Printable>;

typedef std::list<SoundFileInfo> SoundFileInfoList;
typedef std::map<SoundName, SoundFileInfoList> SoundInfoMap;

// Alias Map

typedef std::tuple<SoundName, SoundName, bool> AliasMapEntry;
typedef std::list<AliasMapEntry> AliasMap;

// Misc / Generic

typedef std::vector<std::filesystem::directory_entry> DirectoryEntries;
typedef std::vector<std::filesystem::path> PathList;
typedef std::unordered_map<std::string, int> SoundCache;
typedef std::unordered_map<std::string, bool> MissingSoundCacheFiles;

class CPP_AVFormatContext {
    AVFormatContext *ptr;
    public: CPP_AVFormatContext() {
        ptr = avformat_alloc_context();
        if (ptr == NULL) {
            throw 51;
        }
    }
    bool operator !() const { return ptr == NULL; }
    AVFormatContext* get() { return ptr; };
    AVFormatContext** get_ptr() { return &ptr; };
    CPP_AVFormatContext(CPP_AVFormatContext&&) = default;
    CPP_AVFormatContext& operator=(CPP_AVFormatContext&&) = default;
    CPP_AVFormatContext(const CPP_AVFormatContext&) = delete;
    CPP_AVFormatContext& operator=(const CPP_AVFormatContext&) = delete;
    ~CPP_AVFormatContext() {
        if (ptr)
            avformat_close_input (&ptr);
    }
};

#endif
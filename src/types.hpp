#include <named_type.hpp>

using Duration = fluent::NamedType<double, struct DurationParameter>;
using Samplerate = fluent::NamedType<float, struct SammplerateParameter>;

class SoundDescriptor {
    public:
        explicit SoundDescriptor (Duration d, Samplerate s) : duration_(d), samplerate_(s) {}
    private:
        Duration duration_;
        Samplerate samplerate_;
};

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
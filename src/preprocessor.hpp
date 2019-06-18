#include <filesystem.hpp>

class Preprocessor {
    public:
        explicit Preprocessor (std::filesystem::path);
    private:
        static bool avformat_init;
        static void InitLibAV();
};
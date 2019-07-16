#include <filesystem.hpp>
#include "types.hpp"

class Preprocessor {
    public:
        explicit Preprocessor (std::filesystem::path);

        SoundInfoMap ProcessSoundSet (const std::filesystem::path&);
        bool UpdateSoundSet(const std::filesystem::path&, const int&, const int&);

    private:
        static bool avformat_init;
        static void InitLibAV();

        AliasMap ParseAliasMap (const std::istream&);
};
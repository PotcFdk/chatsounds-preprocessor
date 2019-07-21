#include <filesystem.hpp>
#include "types.hpp"

class Preprocessor {
    public:
        explicit Preprocessor (std::filesystem::path);

        SoundInfoMap ProcessSoundSet (const std::filesystem::path&);
        bool UpdateSoundSet(const std::filesystem::path&, const int&, const int&);

        std::optional<SoundProperties> GetSoundProperties (const std::filesystem::path&);

        AliasMap ParseAliasMap (std::istream&);
        std::optional<SoundFileInfo> GetSoundFileInfo (const std::filesystem::path&);
        SoundFileInfoList ProcessSoundGroup (const std::filesystem::path&);

    private:
        static bool avformat_init;
        static void InitLibAV();

        std::filesystem::path path;
};
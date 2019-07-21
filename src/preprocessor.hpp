#ifndef CP_PREPROCESSOR_HPP
#define CP_PREPROCESSOR_HPP

#include <filesystem.hpp>
#include "types.hpp"

Repository gen_Repository (const std::filesystem::path&);
SoundInfoMap gen_SoundInfoMap (const DirectoryEntries&);

#endif
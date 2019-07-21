#ifndef CP_PREPROCESSOR_HPP
#define CP_PREPROCESSOR_HPP

#include <filesystem.hpp>
#include "types.hpp"

SoundName get_sound_name_from_SFIL (const SoundFileInfoList&);

SoundInfoMap proc_merge_SFIL_into_SIM (SoundInfoMap, SoundFileInfoList);
SoundInfoMap proc_apply_AME_to_SIM (SoundInfoMap, const AliasMapEntry&);

Repository gen_Repository (const std::filesystem::path&);
SoundInfoMap gen_SoundInfoMap (const DirectoryEntries&);
SoundInfoMap gen_SoundInfoMap (const std::filesystem::directory_entry&);

#endif
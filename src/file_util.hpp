#ifndef CP_FILE_UTIL_HPP
#define CP_FILE_UTIL_HPP

#include <filesystem.hpp>
#include "types.hpp"

DirectoryEntries scandir (const std::filesystem::path&);
std::pair <DirectoryEntries, DirectoryEntries> split_files_directories (const DirectoryEntries&);
void EmptyDirectory (const std::filesystem::path&);
void CleanUpEmptySubDirectories (std::filesystem::path&);

#endif
#ifndef CP_FILE_UTIL_HPP
#define CP_FILE_UTIL_HPP

#include <filesystem.hpp>
#include "types.hpp"

DirectoryEntries scandir (const std::filesystem::path&);
std::pair <DirectoryEntries, DirectoryEntries> split_files_directories (const DirectoryEntries&);
std::filesystem::path directory_entry_to_path (const std::filesystem::directory_entry&);
PathList directory_entries_to_pathlist (const DirectoryEntries&);
void EmptyDirectory (const std::filesystem::path&);
void CleanUpEmptySubDirectories (std::filesystem::path&);

#endif
#include <filesystem.hpp>
#include "types.hpp"

DirectoryEntries scandir (const std::filesystem::path& p) {
    DirectoryEntries list;
    for (auto& p : std::filesystem::directory_iterator (p)) {
        list.push_back (p);
    }
    return list;
}

std::pair <DirectoryEntries, DirectoryEntries> split_files_directories (const DirectoryEntries& paths) {
    DirectoryEntries files, dirs;
    std::copy_if (paths.begin(), paths.end(), std::back_inserter(files), [](const std::filesystem::directory_entry& entry) {
        return entry.is_regular_file();
    });
    std::copy_if (paths.begin(), paths.end(), std::back_inserter(dirs), [](const std::filesystem::directory_entry& entry) {
        return entry.is_directory();
    });
    return std::make_pair (files, dirs);
}

void EmptyDirectory (const std::filesystem::path& path) {
    for (std::filesystem::directory_iterator it(path); it != std::filesystem::directory_iterator(); ++it) {
        std::filesystem::remove_all(*it);
    }
}

void CleanUpEmptySubDirectories (std::filesystem::path& path) {
    for (std::filesystem::directory_iterator it(path); it != std::filesystem::directory_iterator(); ++it) {
        if (is_directory(it->status())) {
            std::filesystem::path pp = it->path();
            CleanUpEmptySubDirectories (pp);

            if(std::filesystem::is_empty(pp)) {
                std::filesystem::remove_all(pp);
            }
        }
    }
}
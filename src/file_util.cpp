#include <filesystem.hpp>

void EmptyDirectory (const std::filesystem::path& path) {
    for (std::filesystem::directory_iterator it(path); it != std::filesystem::directory_iterator(); ++it) {
        std::filesystem::remove_all(*it);
    }
}

void CleanUpEmptySubDirectories (std::filesystem::path& path)
{
    for (std::filesystem::directory_iterator it(path); it != std::filesystem::directory_iterator(); ++it)
    {
        if (is_directory(it->status()))
        {
            std::filesystem::path pp = it->path();
            CleanUpEmptySubDirectories (pp);

            if(std::filesystem::is_empty(pp))
            {
                std::filesystem::remove_all(pp);
            }
        }
    }
}
#include <src/file_util.hpp>
#include <catch2/catch.hpp>

using Catch::Matchers::Equals, Catch::Matchers::UnorderedEquals, Catch::Matchers::Contains;

SCENARIO ("scandir behaves correctly", "[file_util]") {
    GIVEN ("a valid directory with entries") {
        std::filesystem::path dir ("../tests/util");
        WHEN ("scandir is called on it") {
            DirectoryEntries entries = scandir (dir);
            THEN ("the result contains all existing entries") {
                PathList pl;
                pl.resize (entries.size());
                std::transform (entries.begin(), entries.end(), pl.begin(), [](std::filesystem::directory_entry& p) {
                    return p.path().filename();
                });
                PathList existing_entries {
                    "file1", "file2", "dir1", "dir2"
                };
                REQUIRE_THAT (pl, Contains(existing_entries));
                AND_THEN ("the result contains only the existing entries") {
                    REQUIRE_THAT (pl, UnorderedEquals(existing_entries));
                }
            }
        }
    }
}

SCENARIO ("split_files_directories behaves correctly", "[file_util]") {
    GIVEN ("a valid PathList") {
        DirectoryEntries entries = scandir ("../tests/util");
        
        PathList _pl;
        std::transform (entries.begin(), entries.end(), std::back_inserter(_pl), [](std::filesystem::directory_entry& p) {
            return p.path().filename();
        });
        REQUIRE_THAT (_pl, UnorderedEquals (PathList {
            "file1", "file2", "dir1", "dir2"
        }));

        WHEN ("split_files_directories is called on it") {
            std::pair <DirectoryEntries, DirectoryEntries> files_dirs = split_files_directories (entries);
            THEN ("the result contains the correct files") {
                PathList files;
                std::transform (files_dirs.first.begin(), files_dirs.first.end(), std::back_inserter(files), [](std::filesystem::directory_entry& p) {
                    return p.path().filename();
                });
                REQUIRE_THAT (files, Equals (PathList {"file1", "file2"}));
            }
            THEN ("the result contains the correct directories") {
                PathList directories;
                std::transform (files_dirs.second.begin(), files_dirs.second.end(), std::back_inserter(directories), [](std::filesystem::directory_entry& p) {
                    return p.path().filename();
                });
                REQUIRE_THAT (directories, Equals (PathList {"dir1", "dir2"}));
            }
        }
    }
}
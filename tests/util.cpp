#include <catch2/catch.hpp>

#include <src/util.cpp>

SCENARIO ("GetSoundDescriptor behaves correctly", "[util]" ) {
    GIVEN ("a path") {
        WHEN ("the path is invalid") {
            std::filesystem::path path ("NONEXISTINGFILE.NAME");
            THEN ("no value is returned") {
                REQUIRE_FALSE (GetSoundDescriptor(path).has_value());
            }
        }
    }
}

SCENARIO ("strip_root behaves correctly", "[util]" ) {
    GIVEN ("a nested path") {
        WHEN ("the path has 3 levels") {
            std::filesystem::path path ("level1/level2/level3");
            THEN ("it strips the first level") {
                REQUIRE (strip_root (path) == "level2/level3");
            }
        }
        WHEN ("the path has one level") {
            std::filesystem::path path ("level");
            THEN ("it strips everything") {
                REQUIRE (strip_root (path) == "");
            }
        }
        WHEN ("the path is empty") {
            std::filesystem::path path ("");
            THEN ("it stays empty") {
                REQUIRE (strip_root (path) == "");
            }
        }
    }
}

SCENARIO ("cmp_ifspath behaves correctly", "[util]" ) {
    GIVEN ("two paths") {
        std::filesystem::path path_1 = "apath1";
        WHEN ("the paths are both identical") {
            std::filesystem::path path_2 = path_1;
            THEN ("it returns 0") {
                REQUIRE (cmp_ifspath (path_1, path_2) == 0);
            }
        }
        WHEN ("the paths are both the same, but differ in the case") {
            std::filesystem::path path_2 = "APaTh1";
            THEN ("it returns 0") {
                REQUIRE (cmp_ifspath (path_1, path_2) == 0);
            }
        }
        WHEN ("the paths differ in content") {
            AND_WHEN ("path_2 comes after path_1") {
                std::filesystem::path path_2 = "apath2";
                THEN ("it returns true") {
                    REQUIRE (cmp_ifspath (path_1, path_2));
                }
            }
            AND_WHEN ("path_2 comes before path_1") {
                std::filesystem::path path_2 = "apath0";
                THEN ("it returns false") {
                    REQUIRE_FALSE (cmp_ifspath (path_1, path_2));
                }
            }
        }
    }
    GIVEN ("a vector of mixed-case paths") {
        std::vector <std::filesystem::path> paths {
            "aPath3/file.dat",
            "apath1/file.dat",
            "ApaTh4/file.dat",
            "apath2/file.dat",
            "apath1/file.dat"
        };
        THEN ("std::sort can sort the paths correctly") {
            std::sort(paths.begin(), paths.end(), cmp_ifspath);
            REQUIRE (paths[0] == "apath1/file.dat");
            REQUIRE (paths[1] == "apath1/file.dat");
            REQUIRE (paths[2] == "apath2/file.dat");
            REQUIRE (paths[3] == "aPath3/file.dat");
            REQUIRE (paths[4] == "ApaTh4/file.dat");
        }
    }
}

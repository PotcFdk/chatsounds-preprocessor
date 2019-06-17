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

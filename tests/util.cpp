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
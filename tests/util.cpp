#include <catch2/catch.hpp>

#include <src/util.hpp>

using Catch::Matchers::Equals, Catch::Matchers::Contains;

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
        std::filesystem::path path_1 ("apath1");
        WHEN ("the paths are both identical") {
            std::filesystem::path path_2 = path_1;
            THEN ("it returns 0") {
                REQUIRE (cmp_ifspath (path_1, path_2) == 0);
            }
        }
        WHEN ("the paths are both the same, but differ in the case") {
            std::filesystem::path path_2 ("APaTh1");
            THEN ("it returns 0") {
                REQUIRE (cmp_ifspath (path_1, path_2) == 0);
            }
        }
        WHEN ("the paths differ in content") {
            AND_WHEN ("path_2 comes after path_1") {
                std::filesystem::path path_2 ("apath2");
                THEN ("it returns true") {
                    REQUIRE (cmp_ifspath (path_1, path_2));
                }
            }
            AND_WHEN ("path_2 comes before path_1") {
                std::filesystem::path path_2 ("apath0");
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
            REQUIRE_THAT (paths, Equals (std::vector <std::filesystem::path> {
                "apath1/file.dat",
                "apath1/file.dat",
                "apath2/file.dat",
                "aPath3/file.dat",
                "ApaTh4/file.dat"
            }));
        }
    }
}

SCENARIO ("getNumberOfDirectories behaves correctly", "[util]") {
    GIVEN ("the test path") {
        std::filesystem::path path ("temp");
        THEN ("the correct number of directories is returned") {
            REQUIRE (getNumberOfDirectories (path) == 1);
        }
    }
}

SCENARIO ("cmp_sfi behaves correctly", "[util]") {
    GIVEN ("two SoundFileInfo objects") {
        SoundFileInfo o1 ("apath1", Duration(0), Samplerate(0));
        WHEN ("the paths are both identical") {
            SoundFileInfo o2 ("apath1", Duration(0), Samplerate(0));
            THEN ("it returns 0") {
                REQUIRE (cmp_sfi (o1, o2) == 0);
            }
        }
        WHEN ("the paths are both the same, but differ in the case") {
            SoundFileInfo o2 ("APaTh1", Duration(0), Samplerate(0));
            THEN ("it returns 0") {
                REQUIRE (cmp_sfi (o1, o2) == 0);
            }
        }
        WHEN ("the paths differ in content") {
            AND_WHEN ("path_2 comes after path_1") {
                SoundFileInfo o2 ("apath2", Duration(0), Samplerate(0));
                THEN ("it returns true") {
                    REQUIRE (cmp_sfi (o1, o2));
                }
            }
            AND_WHEN ("path_2 comes before path_1") {
                SoundFileInfo o2 ("apath0", Duration(0), Samplerate(0));
                THEN ("it returns false") {
                    REQUIRE_FALSE (cmp_sfi (o1, o2));
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
            REQUIRE_THAT (paths, Equals (std::vector <std::filesystem::path> {
                "apath1/file.dat",
                "apath1/file.dat",
                "apath2/file.dat",
                "aPath3/file.dat",
                "ApaTh4/file.dat"
            }));
        }
    }
}

SCENARIO ("is_upper behaves correctly", "[utils]") {
    GIVEN ("is_upper") {
        is_upper o;
        WHEN ("a character is lowercase") {
            char c = 'a';
            THEN ("is_upper returns false") {
                REQUIRE (o(c) == false);
            }
        }
        WHEN ("a character is uppercase") {
            char c = 'D';
            THEN ("is_upper returns true") {
                REQUIRE (o(c) == true);
            }
        }
        WHEN ("a string contains no uppercase characters") {
            std::string str = "hello, world!";
            THEN ("std::any can use is_upper and returns false") {
                REQUIRE (any_of(str.begin(), str.end(), o) == false);
            }
        }
        WHEN ("a string contains uppercase characters") {
            std::string str = "hello, World!";
            THEN ("std::any can use is_upper and returns true") {
                REQUIRE (any_of(str.begin(), str.end(), o) == true);
            }
        }
    }
}

SCENARIO ("match_char behaves correctly", "[utils]") {
    GIVEN ("match_char ('f')") {
        match_char o ('f');
        WHEN ("a character is 'f'") {
            char c = 'f';
            THEN ("match_char returns true") {
                REQUIRE (o(c) == true);
            }
        }
        WHEN ("a character is not 'f'") {
            char c = 'F';
            THEN ("match_char returns false") {
                REQUIRE (o(c) == false);
            }
        }
    }
}

SCENARIO ("intDigits behaves correctly", "[utils]") {
    GIVEN (0) {
        THEN ("intDigits returns 1") {
            REQUIRE (intDigits (0) == 1);
        }
    }
    GIVEN (3) {
        THEN ("intDigits returns 1") {
            REQUIRE (intDigits (3) == 1);
        }
    }
    GIVEN (10) {
        THEN ("intDigits returns 2") {
            REQUIRE (intDigits (10) == 2);
        }
    }
    GIVEN (99) {
        THEN ("intDigits returns 2") {
            REQUIRE (intDigits (99) == 2);
        }
    }
    GIVEN (100) {
        THEN ("intDigits returns 3") {
            REQUIRE (intDigits (100) == 3);
        }
    }
}

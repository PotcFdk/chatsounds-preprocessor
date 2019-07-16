#include <catch2/catch.hpp>
#include <src/preprocessor.cpp>

SCENARIO ("GetSoundProperties", "[preprocessor]" ) {
    GIVEN ("a preprocessor") {
        Preprocessor preprocessor (".");
        AND_GIVEN ("a path") {
            WHEN ("the path is invalid") {
                std::filesystem::path path ("NONEXISTINGFILE.NAME");
                THEN ("no value is returned") {
                    REQUIRE_FALSE (preprocessor.GetSoundProperties(path).has_value());
                }
            }
        }
    }
    // TODO: valid cases
}

SCENARIO ("ParseAliasMap", "[preprocessor]") {
    GIVEN ("a preprocessor") {
        Preprocessor preprocessor (".");
        WHEN ("the input is empty") {
            std::istringstream iss("");
            std::istream& input(iss);
            THEN ("an empty map is returned") {
                REQUIRE (preprocessor.ParseAliasMap (input).size() == 0);
            }
        }
        WHEN ("the input contains one mixed-case entry") {
            std::istringstream iss("N_SrC; n_DST");
            std::istream& input(iss);
            THEN ("a map with size 1 is returned") {
                AliasMap map = preprocessor.ParseAliasMap (input);
                REQUIRE (map.size() == 1);
                AND_THEN ("the map contains the correct all-lowercase entry") {
                    REQUIRE (get<0>(map.front()).get() == "n_src");
                    REQUIRE (get<1>(map.front()).get() == "n_dst");
                    REQUIRE (get<2>(map.front()) == false);
                }
            }
        }
        WHEN ("the input contains 4 mixed-case entries") {
            std::istringstream iss("N_SrC1; n_DST1\nn_srC2 ;N_dst2;  \nn_src3 ; n_dst3\nN_SRC4;N_DST4; ");
            std::istream& input(iss);
            THEN ("a map with size 4 is returned") {
                AliasMap map = preprocessor.ParseAliasMap (input);
                REQUIRE (map.size() == 4);
                AND_THEN ("the map contains the correct all-lowercase entries") {
                    REQUIRE (get<0>(map.front()).get() == "n_src1");
                    REQUIRE (get<1>(map.front()).get() == "n_dst1");
                    REQUIRE (get<2>(map.front()) == false);
                    map.pop_front();
                    REQUIRE (get<0>(map.front()).get() == "n_src2");
                    REQUIRE (get<1>(map.front()).get() == "n_dst2");
                    REQUIRE (get<2>(map.front()) == false);
                    map.pop_front();
                    REQUIRE (get<0>(map.front()).get() == "n_src3");
                    REQUIRE (get<1>(map.front()).get() == "n_dst3");
                    REQUIRE (get<2>(map.front()) == false);
                    map.pop_front();
                    REQUIRE (get<0>(map.front()).get() == "n_src4");
                    REQUIRE (get<1>(map.front()).get() == "n_dst4");
                    REQUIRE (get<2>(map.front()) == false);
                }
            }
        }
        WHEN ("the input contains 4 entries some of which use the replace keyword") {
            std::istringstream iss("N_SrC1; n_DST1; \nn_srC2 ;N_dst2; replace \nn_src3 ; n_dst3\nN_SRC4;N_DST4; re ra replaceee repl ");
            std::istream& input(iss);
            THEN ("a map with size 4 is returned") {
                AliasMap map = preprocessor.ParseAliasMap (input);
                REQUIRE (map.size() == 4);
                AND_THEN ("the map contains the correct entries") {
                    REQUIRE (get<0>(map.front()).get() == "n_src1");
                    REQUIRE (get<1>(map.front()).get() == "n_dst1");
                    REQUIRE (get<2>(map.front()) == false);
                    map.pop_front();
                    REQUIRE (get<0>(map.front()).get() == "n_src2");
                    REQUIRE (get<1>(map.front()).get() == "n_dst2");
                    REQUIRE (get<2>(map.front()) == true);
                    map.pop_front();
                    REQUIRE (get<0>(map.front()).get() == "n_src3");
                    REQUIRE (get<1>(map.front()).get() == "n_dst3");
                    REQUIRE (get<2>(map.front()) == false);
                    map.pop_front();
                    REQUIRE (get<0>(map.front()).get() == "n_src4");
                    REQUIRE (get<1>(map.front()).get() == "n_dst4");
                    REQUIRE (get<2>(map.front()) == true);
                }
            }
        }
        WHEN ("the input contains ugly and invalid data but also 4 valid entries") {
            std::istringstream iss("aasssdf,dd,s,;;;;,\n;;\n#a;n;a\n;\nN_SrC1; n_DST1; \nASDF\n\nn_srC2 ;N_dst2; replace \n####\n;;;;;\nn_src3 ; n_dst3\nN_SRC4;N_DST4; re ra replaceee repl ");
            std::istream& input(iss);
            THEN ("a map with size 4 is returned") {
                AliasMap map = preprocessor.ParseAliasMap (input);
                REQUIRE (map.size() == 4);
                AND_THEN ("the map contains the correct entries") {
                    REQUIRE (get<0>(map.front()).get() == "n_src1");
                    REQUIRE (get<1>(map.front()).get() == "n_dst1");
                    REQUIRE (get<2>(map.front()) == false);
                    map.pop_front();
                    REQUIRE (get<0>(map.front()).get() == "n_src2");
                    REQUIRE (get<1>(map.front()).get() == "n_dst2");
                    REQUIRE (get<2>(map.front()) == true);
                    map.pop_front();
                    REQUIRE (get<0>(map.front()).get() == "n_src3");
                    REQUIRE (get<1>(map.front()).get() == "n_dst3");
                    REQUIRE (get<2>(map.front()) == false);
                    map.pop_front();
                    REQUIRE (get<0>(map.front()).get() == "n_src4");
                    REQUIRE (get<1>(map.front()).get() == "n_dst4");
                    REQUIRE (get<2>(map.front()) == true);
                }
            }
        }
    }
}
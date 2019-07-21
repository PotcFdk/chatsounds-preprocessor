#include <catch2/catch.hpp>
#include <src/preprocessor.cpp>

using std::get;

SCENARIO ("GetSoundProperties", "[preprocessor]" ) {
    GIVEN ("a path") {
        WHEN ("the path is invalid") {
            std::filesystem::path path ("NONEXISTINGFILE.NAME");
            THEN ("no value is returned") {
                REQUIRE_FALSE (getSoundProperties(path).has_value());
            }
        }
    }
    // TODO: valid cases
}

SCENARIO ("ParseAliasMap", "[preprocessor]") {
    GIVEN ("an input") {
        WHEN ("the input is empty") {
            std::istringstream iss("");
            std::istream& input(iss);
            THEN ("an empty map is returned") {
                REQUIRE (parseAliasMap (input).size() == 0);
            }
        }
        WHEN ("the input contains one mixed-case entry") {
            std::istringstream iss("N_SrC; n_DST");
            std::istream& input(iss);
            THEN ("a map with size 1 is returned") {
                AliasMap map = parseAliasMap (input);
                REQUIRE (map.size() == 1);
                AND_THEN ("the map contains the correct all-lowercase entry") {
                    REQUIRE (map.front().getSource() == SoundName ("n_src"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst"));
                    REQUIRE (map.front().getReplace() == false);
                }
            }
        }
        WHEN ("the input contains 4 mixed-case entries") {
            std::istringstream iss("N_SrC1; n_DST1\nn_srC2 ;N_dst2;  \nn_src3 ; n_dst3\nN_SRC4;N_DST4; ");
            std::istream& input(iss);
            THEN ("a map with size 4 is returned") {
                AliasMap map = parseAliasMap (input);
                REQUIRE (map.size() == 4);
                AND_THEN ("the map contains the correct all-lowercase entries") {
                    REQUIRE (map.front().getSource() == SoundName ("n_src1"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst1"));
                    REQUIRE (map.front().getReplace() == false);
                    map.pop_front();
                    REQUIRE (map.front().getSource() == SoundName ("n_src2"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst2"));
                    REQUIRE (map.front().getReplace() == false);
                    map.pop_front();
                    REQUIRE (map.front().getSource() == SoundName ("n_src3"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst3"));
                    REQUIRE (map.front().getReplace() == false);
                    map.pop_front();
                    REQUIRE (map.front().getSource() == SoundName ("n_src4"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst4"));
                    REQUIRE (map.front().getReplace() == false);
                }
            }
        }
        WHEN ("the input contains 4 entries some of which use the replace keyword") {
            std::istringstream iss("N_SrC1; n_DST1; \nn_srC2 ;N_dst2; replace \nn_src3 ; n_dst3\nN_SRC4;N_DST4; re ra replaceee repl ");
            std::istream& input(iss);
            THEN ("a map with size 4 is returned") {
                AliasMap map = parseAliasMap (input);
                REQUIRE (map.size() == 4);
                AND_THEN ("the map contains the correct entries") {
                    REQUIRE (map.front().getSource() == SoundName ("n_src1"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst1"));
                    REQUIRE (map.front().getReplace() == false);
                    map.pop_front();
                    REQUIRE (map.front().getSource() == SoundName ("n_src2"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst2"));
                    REQUIRE (map.front().getReplace() == true);
                    map.pop_front();
                    REQUIRE (map.front().getSource() == SoundName ("n_src3"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst3"));
                    REQUIRE (map.front().getReplace() == false);
                    map.pop_front();
                    REQUIRE (map.front().getSource() == SoundName ("n_src4"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst4"));
                    REQUIRE (map.front().getReplace() == true);
                }
            }
        }
        WHEN ("the input contains ugly and invalid data but also 4 valid entries") {
            std::istringstream iss("aasssdf,dd,s,;;;;,\n;;\n#a;n;a\n;\nN_SrC1; n_DST1; \nASDF\n\nn_srC2 ;N_dst2; replace \n####\n;;;;;\nn_src3 ; n_dst3\nN_SRC4;N_DST4; re ra replaceee repl ");
            std::istream& input(iss);
            THEN ("a map with size 4 is returned") {
                AliasMap map = parseAliasMap (input);
                REQUIRE (map.size() == 4);
                AND_THEN ("the map contains the correct entries") {
                    REQUIRE (map.front().getSource() == SoundName ("n_src1"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst1"));
                    REQUIRE (map.front().getReplace() == false);
                    map.pop_front();
                    REQUIRE (map.front().getSource() == SoundName ("n_src2"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst2"));
                    REQUIRE (map.front().getReplace() == true);
                    map.pop_front();
                    REQUIRE (map.front().getSource() == SoundName ("n_src3"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst3"));
                    REQUIRE (map.front().getReplace() == false);
                    map.pop_front();
                    REQUIRE (map.front().getSource() == SoundName ("n_src4"));
                    REQUIRE (map.front().getDestination() == SoundName ("n_dst4"));
                    REQUIRE (map.front().getReplace() == true);
                }
            }
        }
    }
}

SCENARIO ("get_sound_name_from_SFIL", "[preprocessor]") {
    GIVEN ("a SoundFileInfoList with an entry") {
        SoundFileInfoList sfil {
            SoundFileInfo (std::filesystem::path ("some/dir/file.ogg"), Duration (100), Samplerate (44100))
        };
        THEN ("get_sound_name_from_SFIL returns the correct name") {
            REQUIRE (get_sound_name_from_SFIL (sfil).get() == "file");
        }
    }
}

SCENARIO ("proc_merge_SFIL_into_SIM", "[preprocessor]") {
    GIVEN ("a SoundInfoMap with an entry") {
        SoundInfoMap map { std::make_pair (
                SoundName ("exist"), 
                SoundFileInfoList { SoundFileInfo (std::filesystem::path ("some/dir/exist.ogg"), Duration (100), Samplerate (44100)) }
            )
        };
        REQUIRE (map.find (SoundName ("exist")) != map.end());

        WHEN ("an SFIL with a different name is merged") {
            SoundFileInfoList sfil {
                SoundFileInfo (std::filesystem::path ("some/dir/new.ogg"), Duration (100), Samplerate (44100))
            };
            REQUIRE (get_sound_name_from_SFIL (sfil).get() == "new");
            THEN ("proc_merge_SFIL_into_SIM contains both keys") {
                map = proc_merge_SFIL_into_SIM (map, sfil);
                REQUIRE (map.find (SoundName ("exist")) != map.end());
                REQUIRE (map.find (SoundName ("new")) != map.end());
            }
        }
        WHEN ("an SFIL with the same name is merged") {
            SoundFileInfoList sfil {
                SoundFileInfo (std::filesystem::path ("some/dir/exist/new.ogg"), Duration (100), Samplerate (44100))
            };
            sfil.front().setName (SoundName ("exist"));

            REQUIRE (get_sound_name_from_SFIL (sfil).get() == "exist");
            THEN ("proc_merge_SFIL_into_SIM contains only that key") {
                map = proc_merge_SFIL_into_SIM (map, sfil);
                REQUIRE (map.find (SoundName ("exist")) != map.end());
                REQUIRE (map.find (SoundName ("new")) == map.end());
                AND_THEN ("that key's SFIL contains both SFI objects") {
                    SoundFileInfoList merged_sfil = map[SoundName("exist")];
                    REQUIRE (merged_sfil.front().getPath() == "some/dir/exist/new.ogg");
                    REQUIRE ((++merged_sfil.begin())->getPath() == "some/dir/exist.ogg");
                }
            }
        }
    }
}

SCENARIO ("proc_apply_AME_to_SIM", "[preprocessor]") {
    GIVEN ("a SoundInfoMap with entries") {
        SoundInfoMap map {
            std::make_pair (SoundName ("exist1"), 
                SoundFileInfoList { SoundFileInfo (std::filesystem::path ("some/dir/exist1.ogg"), Duration (100), Samplerate (44100)) }
            ),
            std::make_pair (SoundName ("exist2"), 
                SoundFileInfoList { SoundFileInfo (std::filesystem::path ("some/dir/exist2.ogg"), Duration (100), Samplerate (44100)) }
            ),
            std::make_pair (SoundName ("exist3"), 
                SoundFileInfoList { SoundFileInfo (std::filesystem::path ("some/dir/exist3.ogg"), Duration (100), Samplerate (44100)) }
            )
        };
        REQUIRE (map.find (SoundName ("exist1")) != map.end());
        REQUIRE (map.find (SoundName ("exist2")) != map.end());
        REQUIRE (map.find (SoundName ("exist3")) != map.end());

        AND_GIVEN ("an AliasMapEntry that moves to an empty destination") {
            AliasMapEntry ame (SoundName ("exist1"), SoundName ("existnew"), true);
            THEN ("the AliasMap is applied correctly") {
                map = proc_apply_AME_to_SIM (map, ame);
                REQUIRE (map.find (SoundName ("exist1"))   == map.end());
                REQUIRE (map.find (SoundName ("exist2"))   != map.end());
                REQUIRE (map.find (SoundName ("exist3"))   != map.end());
                REQUIRE (map.find (SoundName ("existnew")) != map.end());
                REQUIRE (map[SoundName ("existnew")].front().getPath() == "some/dir/exist1.ogg");
            }
        }

        AND_GIVEN ("an AliasMapEntry that copies to an empty destination") {
            AliasMapEntry ame (SoundName ("exist1"), SoundName ("existnew"), false);
            THEN ("the AliasMap is applied correctly") {
                map = proc_apply_AME_to_SIM (map, ame);
                REQUIRE (map.find (SoundName ("exist1"))   != map.end());
                REQUIRE (map.find (SoundName ("exist2"))   != map.end());
                REQUIRE (map.find (SoundName ("exist3"))   != map.end());
                REQUIRE (map.find (SoundName ("existnew")) != map.end());
                REQUIRE (map[SoundName ("existnew")].front().getPath() == "some/dir/exist1.ogg");
            }
        }

        AND_GIVEN ("an AliasMapEntry that moves to an existing destination") {
            AliasMapEntry ame (SoundName ("exist1"), SoundName ("exist2"), true);
            THEN ("the AliasMap is applied correctly") {
                map = proc_apply_AME_to_SIM (map, ame);
                REQUIRE (map.find (SoundName ("exist1"))   == map.end());
                REQUIRE (map.find (SoundName ("exist2"))   != map.end());
                REQUIRE (map.find (SoundName ("exist3"))   != map.end());
                REQUIRE (map[SoundName ("exist2")].front().getPath() == "some/dir/exist2.ogg");
                REQUIRE ((++map[SoundName ("exist2")].begin())->getPath() == "some/dir/exist1.ogg");
            }
        }

        AND_GIVEN ("an AliasMapEntry that copies to an existing destination") {
            AliasMapEntry ame (SoundName ("exist1"), SoundName ("exist2"), false);
            THEN ("the AliasMap is applied correctly") {
                map = proc_apply_AME_to_SIM (map, ame);
                REQUIRE (map.find (SoundName ("exist1"))   != map.end());
                REQUIRE (map.find (SoundName ("exist2"))   != map.end());
                REQUIRE (map.find (SoundName ("exist3"))   != map.end());
                REQUIRE (map[SoundName ("exist1")].front().getPath() == "some/dir/exist1.ogg");
                REQUIRE (map[SoundName ("exist2")].front().getPath() == "some/dir/exist2.ogg");
                REQUIRE ((++map[SoundName ("exist2")].begin())->getPath() == "some/dir/exist1.ogg");
            }
        }
    }
}



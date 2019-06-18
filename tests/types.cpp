#include <catch2/catch.hpp>

#include <src/types.hpp>

SCENARIO ("SoundProperties can be constructed and used", "[types]" ) {
    GIVEN ("a SoundProperties object") {
        SoundProperties object (Duration(123), Samplerate(44100));
        THEN ("the duration can be read") {
            REQUIRE (object.getDuration() == 123);
        }
        THEN ("the sample rate can be read") {
            REQUIRE (object.getSamplerate() == 44100);
        }
    }
}

SCENARIO ("SoundFileInfo can be constructed and used", "[types]" ) {
    GIVEN ("a SoundFileInfo object") {
        SoundFileInfo object (std::filesystem::path ("TEST"), Duration(123), Samplerate(44100));
        THEN ("the path can be read") {
            REQUIRE (object.getPath() == "TEST");
        }
        THEN ("the duration can be read") {
            REQUIRE (object.getDuration() == 123);
        }
        THEN ("the sample rate can be read") {
            REQUIRE (object.getSamplerate() == 44100);
        }
    }
}

SCENARIO ("SoundFileInfoLists can be merged") {
    GIVEN ("a SoundInfoMap") {
        SoundInfoMap map;
        AND_GIVEN ("two lists as values") {
            SoundName A("key1");
            SoundName B("key2");
            {
                SoundFileInfoList list;
                list.push_back(SoundFileInfo(std::filesystem::path("TEST2"), Duration(123), Samplerate(0)));
                map[A] = list;
            }
            {
                SoundFileInfoList list;
                list.push_back(SoundFileInfo(std::filesystem::path("TEST6"), Duration(234), Samplerate(0)));
                map[B] = list;
            }
            WHEN ("the lists are merged with .merge()") {
                map[A].merge(map[B]);
                THEN ("it merges the lists correctly") {
                    REQUIRE (map[A].back().getDuration() == 234);
                }
            }
        }
    }
}

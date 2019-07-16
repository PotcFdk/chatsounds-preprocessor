#include <vector>

#define S_TERMINAL_WIDTH 80

#define S_CACHE_VERSION 1
#define S_CACHE_PATH "chatsounds-preprocessor-cache"
#define S_INVALID_FILE_LOG_PATH "invalid-soundfiles.txt"

#define S_LISTPATH "lua/chatsounds/lists_nosend"
#define S_SOUNDPATH "sound/chatsounds/autoadd"
#define S_SOUNDPATH_IGNORELEN 6 // Ignores "sound/"
#define S_SOUNDPATH_MAXLEN 150 // Arbitrary limit enforced by Garry's Mod

#define LIST_DURATION_PRECISION 3 // Precision of the sound lengths in the list e.g. (3.141) -> 3

#define S_BUGTRACKER_LINK "https://github.com/PotcFdk/chatsounds-preprocessor/issues"

const std::vector<unsigned int> valid_samplerates_ogg = {
    11025,
    22050,
    44100,
};

const int TERMINAL_WIDTH = S_TERMINAL_WIDTH;

const unsigned int CACHE_VERSION = S_CACHE_VERSION;
const char * const CACHE_PATH = S_CACHE_PATH;
const char * const INVALID_FILE_LOG_PATH = S_INVALID_FILE_LOG_PATH;

const char * const LISTPATH  = S_LISTPATH;
const char * const SOUNDPATH = S_SOUNDPATH;
const uint_fast8_t SOUNDPATH_IGNORELEN = S_SOUNDPATH_IGNORELEN;
const uint_fast8_t SOUNDPATH_MAXLEN = S_SOUNDPATH_MAXLEN;

const char * const BUGTRACKER_LINK = S_BUGTRACKER_LINK;

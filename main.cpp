/*** You had a choice: looking at this code,  ***
 *** or living in blissful ignorance.         ***
 *** You chose poorly.                        ***/

// Settings

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

/// Includes

#include <src/preprocessor.hpp>
#include <src/types.hpp>
#include <src/error_logger.cpp>
#include <src/util.cpp>
#include <src/file_util.cpp>
#include <src/info.cpp>

#include <cstdint>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

#include "filesystem.hpp"
#include "version.hpp"

// List
#include <tuple>
#include <list>
#include <map>
#include <unordered_map>

// Boost
#include <boost/version.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

using namespace std;

/// Definitions

const int TERMINAL_WIDTH = S_TERMINAL_WIDTH;

const unsigned int CACHE_VERSION = S_CACHE_VERSION;
const char * const CACHE_PATH = S_CACHE_PATH;
const char * const INVALID_FILE_LOG_PATH = S_INVALID_FILE_LOG_PATH;

const char * const LISTPATH  = S_LISTPATH;
const char * const SOUNDPATH = S_SOUNDPATH;
const uint_fast8_t SOUNDPATH_IGNORELEN = S_SOUNDPATH_IGNORELEN;
const uint_fast8_t SOUNDPATH_MAXLEN = S_SOUNDPATH_MAXLEN;

const char * const BUGTRACKER_LINK = S_BUGTRACKER_LINK;

static const char
    *EXT_LINE = "                                                                      | |",
    *RXT_LINE = "                                                                      | |\r",
    *END_LINE = "                                                                      |/",
    *RND_LINE = "                                                                      |/\r",
    *NULL_CHR = "\0";

ErrorLogger error_log (INVALID_FILE_LOG_PATH);

///

unsigned short month, day, year;

const char *months[] = {
    "Jan", "Feb", "Mar", "Apr", "May",
    "Jun", "Jul", "Aug","Sep", "Oct", "Nov", "Dec"
};

void getdate ()
{
    char temp [] = __DATE__;
    unsigned char i;

    year = atoi(temp + 7);
    *(temp + 6) = 0;
    day = atoi(temp + 4);
    *(temp + 3) = 0;
    for (i = 0; i < 12; i++)
    {
        if (!strcmp(temp, months[i]))
        {
            month = i + 1;
            return;
        }
    }
}

///

const std::vector<unsigned int> valid_samplerates_ogg =
{
    11025,
    22050,
    44100,
};


struct is_upper
{
    bool operator() (int value)
    {
        return ::isupper ((unsigned char) value);
    }
};

struct match_char
{
    char c;
    match_char(char c) : c(c) {}
    bool operator()(char x) const
    {
        return x == c;
    }
};


bool is_interactive = true;
inline void interactive_wait_for_any_key() {
    if (is_interactive) {
        cout << endl << "Press ENTER to exit..." << endl;
        cin.get();
    }
}

char * parent_dir = NULL;
bool detectWorkingDir() {
    if (std::filesystem::is_directory("sound") && std::filesystem::is_directory("lua"))
        return false; // The current directory seems to be okay.
    else if (parent_dir) // Let's try the location of the executable, instead?
        std::filesystem::current_path(std::filesystem::absolute(parent_dir).remove_filename());
        return true;
}


Preprocessor preprocessor;



int intDigits (int number)
{
    int digits = 0;
    while (number)
    {
        number /= 10;
        ++digits;
    }
    return digits;
}

void DisplayGenerationActivity(const bool& added, std::string name, const int& folder_p, const int& folder_t, int progress = -1)
{
    // TERMINAL_WIDTH - "() " - STATUS - #P - #N - '/' - LASTCHR
    unsigned short shortn = TERMINAL_WIDTH - 3 - 4 - intDigits(folder_p) - intDigits(folder_t) - 1 - 2;

    cout << '(' << folder_p << '/' << folder_t << ") "
         << (name.size() >= shortn ? "..." + name.substr(name.size() - shortn + 3) : name + string(shortn - name.size(), ' '));

    if (progress != 100 && progress != -1)
        cout << ' ' << progress << " %";

    error_log.reset();
}

bool __gen_activity_added;
std::string __gen_activity_name;
int __gen_activity_folder_p, __gen_activity_folder_t;

std::chrono::high_resolution_clock::time_point __gen_activity_rst  = std::chrono::high_resolution_clock::now();
std::chrono::high_resolution_clock::time_point __gen_activity_last = std::chrono::high_resolution_clock::now();
std::chrono::duration<double> duration;

inline void UpdateGenerationActivity(int progress = -1, bool force = false)
{
    static std::chrono::high_resolution_clock::time_point now;
    now = std::chrono::high_resolution_clock::now();

    duration = now - __gen_activity_last;
    if (duration.count() > 0.5 || force)
    {
        cout << '\r';
        DisplayGenerationActivity(__gen_activity_added, __gen_activity_name, __gen_activity_folder_p, __gen_activity_folder_t, progress);
        __gen_activity_last = now;
    }
}

void SetGenerationActivityParameters(const bool& added, std::string name, const int& folder_p, const int& folder_t)
{
    __gen_activity_added    = added;
    __gen_activity_name     = name;
    __gen_activity_folder_p = folder_p;
    __gen_activity_folder_t = folder_t;
    __gen_activity_last     = __gen_activity_rst;
}



boost::optional<SoundFileInfo> GetSoundFileInfo (const std::filesystem::path& path) // Assembles an infolist about a sound.
{
    {
        const string str_path = path.string();

        // Check path length

        if (str_path.length() > SOUNDPATH_MAXLEN)
        {
            error_log << "[too long path] " << str_path << endl;
            return boost::none;
        }

        // Check if path is all lowercase.

        else if (any_of(str_path.begin(), str_path.end(), is_upper()))
        {
            error_log << "[non-lowercase path] " << str_path << endl;
            return boost::none;
        }
    }

    if (path.has_extension())
    {
        string ext = path.extension().string();
        boost::algorithm::to_lower(ext);

        if (ext == ".ogg" || ext == ".mp3" || ext == ".wav")
        {
            std::filesystem::path full_path = std::filesystem::absolute(path);

            std::optional<SoundProperties> properties = GetSoundProperties (full_path);

            if (properties.has_value() && ext != ".ogg"
                || (std::find(valid_samplerates_ogg.begin(), valid_samplerates_ogg.end(), properties->getSamplerate())
                    != valid_samplerates_ogg.end())
            )
            {
                return SoundFileInfo(strip_root(path), properties.value());
            } else if (properties.has_value()) {
                error_log << "[invalid sample rate] " << path << ": " << properties->getSamplerate() << endl;
            } else {
                error_log << "[invalid file] " << path << endl;
            }
        }
    }
    return boost::none;
}

SoundFileInfoList ProcessSoundGroup (const std::filesystem::path& path)
{
    SoundFileInfoList list;

    PathList paths;
    copy(std::filesystem::directory_iterator(path), std::filesystem::directory_iterator(), back_inserter(paths));
    sort(paths.begin(), paths.end(), cmp_ifspath); // To make sure it's sorted.

    for(PathList::const_iterator it (paths.begin()); it != paths.end(); ++it) {
        std::filesystem::path sub_path = std::filesystem::absolute((*it));

        if (std::filesystem::is_regular_file(sub_path)) {
            if (boost::optional<SoundFileInfo> soundfileinfo = GetSoundFileInfo(*it)) {
                list.push_back(*soundfileinfo);
            }
        }
    }
    return list;
}

bool WriteSoundList(const SoundInfoMap& list, const string& listname)
{
    if (list.empty()) return false;

    std::ofstream f(string(LISTPATH) + "/" + listname + ".lua", std::ofstream::binary);
    if (!f.fail())
    {
        bool first_in_multientry;
        f << "c.StartList(\"" << listname << "\")\n";

        for (SoundInfoMap::const_iterator it = list.begin(); it != list.end(); ++it)
        {
            // info: it[0] ("first") = list name; it[1] ("second") = sound list
            first_in_multientry = true;
            f << "L[\"" << it->first << "\"]={";
            for (SoundFileInfoList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            {
                // info: it2[0] = path; it2[1] = duration
                if (first_in_multientry)
                    first_in_multientry = false;
                else
                    f << ',';

                string path = it2->getPath().string();
                std::replace(path.begin(), path.end(), '\\', '/');

                f << "{path=\"" << path << "\",length="
                  << std::fixed
                  << std::setprecision(LIST_DURATION_PRECISION)
                  << it2->getDuration() << "}";

            }
            f << "}\n";
        }

        f << "c.EndList()";
        f.close();
        return true;
    }
    return false;
}

void UpdateSoundSet(const string& name, const int& folder_p, const int& folder_t) {
    string list_path = string(LISTPATH) + "/" + name + ".lua";
    std::filesystem::path soundsetpath(string(SOUNDPATH) + "/" + name);

    if (std::filesystem::is_directory(soundsetpath)) { // If the directory exists.
        if (!UpdateSoundFolder(soundsetpath, folder_p, folder_t)) {
            std::filesystem::remove(list_path); // broken sound set
        }
    }
    else if (std::filesystem::is_regular_file(list_path)) { // If a related (unneeded/outdated!) sound list exists.
        DisplayGenerationActivity(false, name, folder_p, folder_t);
        std::filesystem::remove(list_path);
        cout << " done" << endl;
    }
}

void UpdateSoundSets(const unordered_map<string, bool>& SoundCacheDiff) {
    int i = 1;
    for(auto it = SoundCacheDiff.begin(); it != SoundCacheDiff.end(); ++it) {
        UpdateSoundSet(it->first, i, SoundCacheDiff.size());
        i++;
    }
}

MissingSoundCacheFiles GetModifiedSoundSets(const SoundCache& cache1, const SoundCache& cache2) {
    unordered_map<string, bool> list;
    bool diff;

    // Add differences to list:

    for (auto it = cache1.begin(); it != cache1.end(); ++it)
    {
        try
        {
            diff = cache2.at(it->first) != it->second;
        }
        catch (std::out_of_range e)
        {
            diff = true;
        }

        if (diff)
        {
            std::filesystem::path path(it->first);

            int i = 0;

            while (path.parent_path().filename() != "autoadd")
            {
                path = path.parent_path();

                if (i > 300)
                    throw 31;
                else
                    i++;
            }

            list[path.filename().string()] = true;
        }
    }

    for (auto it = cache2.begin(); it != cache2.end(); ++it)
    {
        try
        {
            diff = cache1.at(it->first) != it->second;
        }
        catch (std::out_of_range e)
        {
            diff = true;
        }

        if (diff)
        {
            std::filesystem::path path(it->first);

            int i = 0;

            while (path.parent_path().filename() != "autoadd")
            {
                path = path.parent_path();

                if (i > 300)
                    throw 32;
                else
                    i++;
            }

            list[path.filename().string()] = true;
        }
    }

    return list;
}

void AddMissingLists(MissingSoundCacheFiles& list, const SoundCache& soundcache)
{
    for ( auto it = soundcache.begin(); it != soundcache.end(); ++it )
    {
        std::filesystem::path path(it->first);

        int i = 0;

        while (path.parent_path().filename() != "autoadd")
        {
            path = path.parent_path();

            if (i > 300)
                throw 33;
            else
                i++;
        }

        string name = path.filename().string();
        string list_path = string(LISTPATH) + "/" + name + ".lua";
        if (!std::filesystem::is_regular_file(list_path))
        {
            list[name] = true;
        }
    }
}

// Sound Cache

void EraseSoundCache()
{
    try
    {
        if (std::filesystem::exists(CACHE_PATH))
            std::filesystem::remove(CACHE_PATH);
    }
    catch (boost::archive::archive_exception e)
    {
        cout << "Boost exception: " << e.what() << endl;
    }
}

SoundCache ReadSoundCache()
{
    if (!std::filesystem::exists(CACHE_PATH))
        throw 11;

    std::ifstream ifs(CACHE_PATH, ios::binary);
    boost::archive::binary_iarchive ia(ifs);

    unsigned int file_cache_version;
    ia & file_cache_version;

    if (file_cache_version != CACHE_VERSION)
    {
        ifs.close();
        throw 12;
    }

    unsigned int cache_size = 0;
    ia & cache_size;


    SoundCache soundcache;
    for (unsigned int i = 0; i < cache_size; i+=1)
    {
        string path;
        int timestamp;
        ia & path;
        ia & timestamp;
        soundcache[path] = timestamp;
    }
    ifs.close();
    return soundcache;
}

SoundCache GenerateSoundCache()
{
    SoundCache soundcache;

    int cnt = 1, cnt_total = getNumberOfDirectories(SOUNDPATH);

    for (std::filesystem::directory_iterator it(SOUNDPATH); it != std::filesystem::directory_iterator(); ++it)
    {
        if (is_directory(it->status()))
        {
            for (std::filesystem::directory_iterator its(it->path()); its != std::filesystem::directory_iterator(); ++its)
            {
                if (is_directory(its->status()))
                {
                    for (std::filesystem::directory_iterator itg(its->path()); itg != std::filesystem::directory_iterator(); ++itg)
                    {
                        if (std::filesystem::is_regular_file(itg->status()))
                        {
                            soundcache[itg->path().string()]
                                = std::filesystem::last_write_time(std::filesystem::absolute(itg->path())).time_since_epoch().count();
                        }
                    }
                }
                else
                {
                    soundcache[its->path().string()]
                        = std::filesystem::last_write_time(std::filesystem::absolute(its->path())).time_since_epoch().count();
                }
            }
            cout << '\r' << "Scanning sounds... " << cnt++ << '/' << cnt_total << flush;
        }
    }

    cout << " OK";

    return soundcache;
}

void WriteSoundCache(SoundCache soundcache)
{
    std::ofstream ofs(CACHE_PATH, ios::binary);
    boost::archive::binary_oarchive oa(ofs);

    const unsigned int cache_version = CACHE_VERSION;
    oa << cache_version;
    const unsigned int cache_size = soundcache.size();
    oa << cache_size;

    for ( auto it = soundcache.begin(); it != soundcache.end(); ++it )
    {
        oa << it->first;
        oa << it->second;
    }

    ofs.close();
}



// Modes

int DiffUpdate(const bool &open_ext)
{
    const char * const ln_base   = open_ext ? RXT_LINE : NULL_CHR,
               * const ln_base_e = open_ext ? RND_LINE : NULL_CHR;

    std::chrono::high_resolution_clock::time_point time_begin = std::chrono::high_resolution_clock::now();

    cout << ln_base;

    cout << ln_base << "Resetting invalid soundfile log..." << endl;

    try
    {
        std::filesystem::remove(INVALID_FILE_LOG_PATH);
    }
    catch (std::filesystem::filesystem_error e)
    {
        cout << ln_base << "Cannot reset invalid soundfile log: " << endl << "  " << e.what() << endl << endl;
    }

    cout << ln_base << "Cleaning up sound folder..." << endl;
    try
    {
        CleanUpEmptySubDirectories (SOUNDPATH);
    }
    catch(std::filesystem::filesystem_error e)
    {
        cout << ln_base << "Boost exception: " << e.what() << endl;
        throw 1;
    }

    cout << ln_base << "Reading sound cache..." << endl;
    SoundCache soundcache;
    try
    {
        soundcache = ReadSoundCache();
    }
    catch (boost::archive::archive_exception e)
    {
        cout << ln_base << "Boost exception: " << e.what() << endl;
        EraseSoundCache();
    }
    catch (int e)
    {
        if (e == 11) {} // No cache file.
        else if (e == 12)
            cout << ln_base << "Incompatible sound cache, reset." << endl;
        else
            cout << ln_base << "ERROR " << e << endl;
        EraseSoundCache();
    }

    cout << ln_base << "Scanning sounds...";

    SoundCache new_soundcache;
    MissingSoundCacheFiles to_be_updated;

    try
    {
        new_soundcache = GenerateSoundCache();
        cout << endl << ln_base_e << "Calculating difference..." << flush;
        to_be_updated = GetModifiedSoundSets(soundcache, new_soundcache);
        AddMissingLists(to_be_updated, new_soundcache);
    }
    catch (std::filesystem::filesystem_error e)
    {
        cout << "  ERR" << endl
             << "Boost exception: " << e.what() << endl;
        throw 60;
    }

    cout << "  OK" << endl << endl;

    UpdateSoundSets(to_be_updated);
    WriteSoundCache(new_soundcache);

    std::chrono::duration<double> duration (std::chrono::high_resolution_clock::now() - time_begin);

    cout << "List generation has finished. Took " << duration.count() << " seconds." << endl;

    if (std::filesystem::exists(INVALID_FILE_LOG_PATH))
    {
        cout << endl << "[Information] "
             "Some invalid files were found during the generation." << endl
             << "Please open '" << INVALID_FILE_LOG_PATH << "' and double-check these files." << endl
             << "They might be corrupt, empty or have an unsupported sample rate or path." << endl
             << "If you have confirmed they work in-game and believe this is an error, visit" << endl
             << "  " << BUGTRACKER_LINK << endl << "and post a bug report." << endl;
        interactive_wait_for_any_key();
        return -20;
    }

    return 0;
}

int FullUpdate(const bool &open_ext)
{
    const char * const ln_base = open_ext ? RXT_LINE : NULL_CHR;

    cout << ln_base << "Resetting cache..." << endl;

    try
    {
        std::filesystem::remove(CACHE_PATH);
    }
    catch (std::filesystem::filesystem_error e)
    {
        cout << ln_base << "Cannot reset cache: " << endl << "  " << e.what() << endl << endl;
    }

    cout << ln_base << "Deleting old lists..." << endl;
    try
    {
        EmptyDirectory (LISTPATH);
    }
    catch(std::filesystem::filesystem_error e)
    {
        cout << ln_base << "Boost exception: " << e.what() << endl;
        throw 3;
    }

    return DiffUpdate(open_ext);
}



void showError(int e)
{
    cout << endl << "Preprocessor Exception: ERROR " << e << endl;

    if (e >= 30)
        cout << "Please report this bug at:" << endl << "  " << BUGTRACKER_LINK << endl;
    else
        cout << "The error message above should give you an idea on what might be wrong." << endl;

    interactive_wait_for_any_key();
}



int Launch_DiffUpdate(const bool &open_ext)
{
    const char * const ln_base = open_ext ? RXT_LINE : NULL_CHR;

    cout << ln_base << "Running in DIFF mode." << endl;
    if (detectWorkingDir())
        cout << ln_base << "Switched working directory to executable path." << endl;

    try
    {
        return DiffUpdate(open_ext);
    }
    catch (int e)
    {
        showError(e);
        return -e;
    }
    catch (...)
    {
        showError(99);
        return -99;
    }
}

int Launch_FullUpdate(const bool &open_ext)
{
    const char * const ln_base = open_ext ? RXT_LINE : NULL_CHR;

    cout << ln_base << "Running in FULL mode." << endl;
    if (detectWorkingDir())
        cout << ln_base << "Switched working directory to executable path." << endl;

    try
    {
        return FullUpdate(open_ext);
    }
    catch (int e)
    {
        showError(e);
        return -1;
    }
    catch (...)
    {
        showError(99);
        return -99;
    }
}




pair<string, string> win_help_cmd_param(const string& s)
{
    if (s == "/?") {
        return make_pair(string("help"), string());
    } else {
        return make_pair(string(), string());
    }
}

int main(int argc, char* argv[])
{
    parent_dir = *argv;

    boost::program_options::options_description commands("Commands");
    commands.add_options()
        ("help,h", "Usage help")
        ("version,v", "Show the program version")
        ("full,f", "Full, uncached list generation")
        ("diff,d", "Normal, cached list generation (default)")
        ("lite,l", "Same as --diff")
    ;

    boost::program_options::options_description parameters("Parameters");
    parameters.add_options()
        ("non-interactive", "non-interactive mode (don't expect any user input)")
    ;

    boost::program_options::options_description options("Allowed options");
    options.add(commands).add(parameters);

    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
            .options(options).extra_parser(win_help_cmd_param).run(), vm);
        boost::program_options::notify(vm);
    } catch (boost::program_options::unknown_option e) {
        const bool open_ext = print_topinfo();
        cout << (open_ext ? RXT_LINE : NULL_CHR) << e.what() << endl
             << (open_ext ? RND_LINE : NULL_CHR) << "For usage help, see -h or --help." << endl;
        return -4;
    }

    if (vm.count("version"))
    {
        print_versioninfo();
        return 0;
    }

    if (vm.count("non-interactive"))
    {
        is_interactive = false;
    }

    const bool open_ext = print_topinfo();

    const char * const ln_base   = open_ext ? RXT_LINE : NULL_CHR,
               * const ln_base_e = open_ext ? RND_LINE : NULL_CHR;

    if (vm.count("full"))
        return Launch_FullUpdate(open_ext);
    else if (vm.count("diff") || vm.count("lite"))
        return Launch_DiffUpdate(open_ext);
    else if (vm.count("help"))
    {
        cout << ln_base   << "Usage: " << endl
             << ln_base   << " -f | --full     -  Full, uncached list generation" << endl
             << ln_base   << " -d | --diff     -  Normal, cached list generation (default)" << endl
             << ln_base   << " -l | --lite     -  Same as --diff" << endl
             << ln_base   << " -h | --help     -  Usage help (this text right here)" << endl
             << ln_base_e << " -v | --version  -  Show the program version" << endl;
    }
    else
        return Launch_DiffUpdate(open_ext);

    return 0;
}

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

#define LIST_DURATION_PRECISION 3 // Precision of the sound lengths in the list e.g. (3.141) -> 3

#define S_BUGTRACKER_LINK "https://github.com/PotcFdk/chatsounds-preprocessor/issues"

/// Includes

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
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

// libavformat
extern "C" {
#include <libavformat/avformat.h>
}

using namespace std;

/// Definitions

const int TERMINAL_WIDTH = S_TERMINAL_WIDTH;

const unsigned int CACHE_VERSION = S_CACHE_VERSION;
const char * const CACHE_PATH = S_CACHE_PATH;
const char * const INVALID_FILE_LOG_PATH = S_INVALID_FILE_LOG_PATH;

const char * const LISTPATH  = S_LISTPATH;
const char * const SOUNDPATH = S_SOUNDPATH;
const uint_fast8_t SOUNDPATH_IGNORELEN = S_SOUNDPATH_IGNORELEN;

const char * const BUGTRACKER_LINK = S_BUGTRACKER_LINK;

static const char
    *EXT_LINE = "                                                                      | |",
    *RXT_LINE = "                                                                      | |\r",
    *END_LINE = "                                                                      |/",
    *RND_LINE = "                                                                      |/\r",
    *NULL_CHR = "\0";


typedef vector<std::filesystem::path> PathList;

typedef tuple<string, string, bool> SoundMapEntry;
typedef list<SoundMapEntry> SoundMap;

typedef pair<std::filesystem::path, double> SoundInfo;
typedef list<SoundInfo> SoundInfos;
typedef map<string, SoundInfos> SoundInfoMap;

typedef unordered_map<string, int> SoundCache;
typedef unordered_map<string, bool> MissingSoundCacheFiles;

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

ofstream invalid_file_log;
void invalid_file_log_open()
{
    if (!invalid_file_log.is_open())
        invalid_file_log.open(INVALID_FILE_LOG_PATH);
}
void invalid_file_log_close()
{
    invalid_file_log.close();
}

class ErrorLogger
{
private:
    bool in_line = true;

public:
    void reset () { in_line = true; }

    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    typedef CoutType& (*StandardEndLine)(CoutType&);

    ErrorLogger& operator << (StandardEndLine manip)
    {
        invalid_file_log_open();
        invalid_file_log << std::endl;
        std::cout << std::endl;
        in_line = false;
        return *this;
    }

    template <typename T> ErrorLogger& operator << (const T& value)
    {
        invalid_file_log_open();
        invalid_file_log << value;
        if (in_line)
        {
            std::cout << std::endl;
            in_line = false;
        }
        std::cout << value;
        return *this;
    }
};

ErrorLogger error_log;

bool cmp_ifspath (const std::filesystem::path& first, const std::filesystem::path& second)
{
    return boost::algorithm::ilexicographical_compare(first.c_str(), second.c_str());
}

bool cmp_si (const SoundInfo& first, const SoundInfo& second)
{
    return boost::algorithm::ilexicographical_compare(first.first.c_str(), second.first.c_str());
}

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

std::filesystem::path strip_root(const std::filesystem::path& p) {
    const std::filesystem::path& parent_path = p.parent_path();
    if (parent_path.empty() || parent_path.string() == "/")
        return std::filesystem::path();
    else
        return strip_root(parent_path) / p.filename();
}

bool avformat_init = false;
void InitLibAV()
{
    if (!avformat_init)
    {
        cout << "Initializing libavformat..." << endl;
        av_log_set_level(AV_LOG_ERROR);
        avformat_init = true;
    }
}

bool is_interactive = true;
inline void interactive_wait_for_any_key()
{
    if (is_interactive)
    {
        cout << endl << "Press ENTER to exit..." << endl;
        cin.get();
    }
}

char * parent_dir = NULL;
bool detectWorkingDir()
{
    if (std::filesystem::is_directory("sound") && std::filesystem::is_directory("lua"))
        return false; // The current directory seems to be okay.
    else if (parent_dir) // Let's try the location of the executable, instead?
        std::filesystem::current_path(std::filesystem::absolute(parent_dir).remove_filename());
        return true;
}

int getNumberOfDirectories (std::filesystem::path path)
{
    return std::count_if (std::filesystem::directory_iterator(path),
        std::filesystem::directory_iterator(),
        bind (static_cast<bool(*)(const std::filesystem::path&)> (std::filesystem::is_directory),
            bind (&std::filesystem::directory_entry::path, _1)));
}

int getNumberOfFiles (std::filesystem::path path)
{
    return std::count_if (std::filesystem::directory_iterator(path),
        std::filesystem::directory_iterator(),
        bind (static_cast<bool(*)(const std::filesystem::path&)> (std::filesystem::is_directory),
            bind (&std::filesystem::directory_entry::path, _1)));
}

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

class CPP_AVFormatContext {
    AVFormatContext *ptr;
    public: CPP_AVFormatContext() {
        ptr = avformat_alloc_context();
        if (ptr == NULL) {
            throw 51;
        }
    }
    bool operator !() const { return ptr == NULL; }
    AVFormatContext* get() { return ptr; };
    AVFormatContext** get_ptr() { return &ptr; };
    CPP_AVFormatContext(CPP_AVFormatContext&&) = default;
    CPP_AVFormatContext& operator=(CPP_AVFormatContext&&) = default;
    CPP_AVFormatContext(const CPP_AVFormatContext&) = delete;
    CPP_AVFormatContext& operator=(const CPP_AVFormatContext&) = delete;
    ~CPP_AVFormatContext() {
        if (ptr)
            avformat_close_input (&ptr);
    }
};

inline double GetSoundDuration(const std::filesystem::path& path, float *rate) // Gets the duration of a sound.
{
    CPP_AVFormatContext ps;
    AVFormatContext *_ps = ps.get();
    avformat_open_input (ps.get_ptr(), path.string().c_str(), NULL, NULL);
    if (!ps) return 0;

    avformat_find_stream_info (_ps, NULL);
    int64_t duration = _ps->duration;
    if (duration <= 0) return 0;
    if (_ps->nb_streams != 1) return 0;
    *rate = _ps->streams[0]->codecpar->sample_rate;
    return static_cast<double>(duration)/AV_TIME_BASE;
}

inline std::filesystem::path GetAbsolutePath(const std::filesystem::path& path)
{
/*#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    std::filesystem::path ret ("\\\\?\\");
    ret += std::filesystem::absolute(path);
    ret.make_preferred();
    return ret;
#else*/
    return std::filesystem::absolute(path);
//#endif
}

boost::optional<SoundInfo> GetSoundInfo(const std::filesystem::path& path) // Assembles an infolist about a sound.
{
    {
        const string str_path = path.string();

        // Check if path is all lowercase.

        if (any_of(str_path.begin(), str_path.end(), is_upper()))
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
            std::filesystem::path full_path = GetAbsolutePath(path);

            float rate = 0;
            double duration = GetSoundDuration(full_path, &rate);
            if (
                ext != ".ogg"
                || (std::find(valid_samplerates_ogg.begin(), valid_samplerates_ogg.end(), rate)
                    != valid_samplerates_ogg.end())
            )
            {
                return SoundInfo(strip_root(path), duration);
            }
            else
            {
                error_log << "[invalid sample rate] " << path << ": " << rate << endl;
            }
        }
    }
    return boost::none;
}

SoundInfos ProcessSoundGroup(const std::filesystem::path& path)
{
    SoundInfos list;

    PathList paths;
    copy(std::filesystem::directory_iterator(path), std::filesystem::directory_iterator(), back_inserter(paths));
    sort(paths.begin(), paths.end(), cmp_ifspath); // To make sure it's sorted.

    for(PathList::const_iterator it (paths.begin()); it != paths.end(); ++it)
    {
        std::filesystem::path sub_path = GetAbsolutePath((*it));

        if (std::filesystem::is_regular_file(sub_path))
        {
            if (boost::optional<SoundInfo> soundinfo = GetSoundInfo(*it))
            {
                list.push_back(*soundinfo);
            }
        }
    }
    return list;
}

SoundMap ParseSoundMap(const std::filesystem::path& path)
{
    SoundMap soundmap;

    std::ifstream f(path.string());

    if (!f.fail())
    {
        std::string ln, source, alias, options;
        bool replace;
        int items_size;
        while (std::getline(f, ln))
        {
            boost::algorithm::trim(ln);

            if (!boost::algorithm::starts_with(ln, "#"))
            {
                std::vector<std::string> items;
                boost::algorithm::split(items, ln, match_char(';'));
                items_size = items.size();
                if (items_size == 2 || items_size == 3) // source and alias exist
                {
                    source = boost::algorithm::trim_copy(items.at(0));
                    alias  = boost::algorithm::trim_copy(items.at(1));

                    boost::algorithm::to_lower(source);
                    boost::algorithm::to_lower(alias);

                    replace = false; // default behavior: don't replace, just alias

                    if (items_size == 3)
                    {
                        options = boost::algorithm::trim_copy(items.at(2));
                        boost::algorithm::to_lower(options);
                        if (boost::algorithm::contains(options, "replace"))
                        {
                            replace = true;
                        }
                    }

                    soundmap.emplace_back(make_tuple(source, alias, replace));
                }
            }
        }
        f.close();
    }

    return soundmap;
}

SoundInfoMap ProcessSounds(const std::filesystem::path& path) // Scans a subdirectory and compiles all the soundinfos into a list.
{
    SoundInfoMap list;
    SoundMap soundmap;

    string _info_list_name;

    PathList paths;
    copy(std::filesystem::directory_iterator(path), std::filesystem::directory_iterator(), back_inserter(paths));

    int i = 1, total = paths.size();

    for(PathList::const_iterator it (paths.begin()); it != paths.end(); ++it, ++i)
    {
        std::filesystem::path sub_path = GetAbsolutePath(*it);

        UpdateGenerationActivity(100 * i / total);

        if (is_directory(sub_path)) // It's a sound group.
        {
            SoundInfos info = ProcessSoundGroup(*it);
            _info_list_name = boost::algorithm::to_lower_copy(it->filename().string());
            if (!list.count(_info_list_name))
            { // Doesn't exist, just add it.
                list[_info_list_name] = info;
            }
            else
            { // Exists, merge it.
                list[_info_list_name].splice(list[_info_list_name].begin(), info);
            }
        }
        else if (std::filesystem::is_regular_file(sub_path)) // It's a single file.
        {
            if (boost::iequals(sub_path.filename().string(), "map.txt"))
            {
                soundmap = ParseSoundMap(sub_path);
            }
            else
            {
                if (boost::optional<SoundInfo> soundinfo = GetSoundInfo(*it))
                {
                    SoundInfo info = *soundinfo;
                    _info_list_name = boost::algorithm::to_lower_copy(it->filename().replace_extension("").string());
                    if (!list.count (_info_list_name))
                    { // Doesn't exist, just add it.
                        list[_info_list_name] = SoundInfos {info};
                    }
                    else
                    { // Exists, add it.
                        list[_info_list_name].push_back(info);
                    }
                }
            }
        }
    }

    if (soundmap.size() > 0) // we have a custom sound map
    {
        for (SoundMap::iterator it = soundmap.begin(); it != soundmap.end(); ++it) // iterate over the sound map
        {
            // Remember: *it = [ ( source, alias, (bool) replace ) ]
            if (list.count(get<0>(*it))) // if source exists in the list
            {
                string alias = get<1>(*it);

                if (!list.count(alias)) // the target / aliased entry doesn't exist in the sound set
                {
                    if (get<2>(*it)) // Replace?
                    {
                        list[alias] = move(list[get<0>(*it)]); // list[alias] = source data
                        list.erase(get<0>(*it)); // -> delete the source entry
                    }
                    else
                    {
                        list[alias] = list[get<0>(*it)];
                    }
                }
                else
                {
                    if (get<2>(*it)) // Replace?
                    {
                        list[alias].merge(list[get<0>(*it)]); // merge (move!) source data to list[alias]
                        list.erase(get<0>(*it)); // -> delete the source entry (now empty after the above)
                    }
                    else // don't replace
                    {
                        std::copy(list[get<0>(*it)].begin(), list[get<0>(*it)].end(), // copy source data to list[alias]
                            std::back_insert_iterator<SoundInfos>(list[alias]));
                    }
                }
            } // end if source exists in the list
        } // end iterate over the sound map
    } // end we have a custom sound map

    // Sort SoundInfo elements inside the SoundInfos in our SoundInfoMap.
    for (SoundInfoMap::iterator it = list.begin(); it != list.end(); ++it)
    {
        it->second.sort(cmp_si);
    }

    return list;
}

SoundInfoMap ProcessSoundFolder(const std::filesystem::path& path)
{
    if (is_directory(path))
        return ProcessSounds(path);
    else
        return SoundInfoMap();
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
            for (SoundInfos::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            {
                // info: it2[0] = path; it2[1] = duration
                if (first_in_multientry)
                    first_in_multientry = false;
                else
                    f << ',';

                string path = it2->first.string();
                std::replace(path.begin(), path.end(), '\\', '/');

                f << "{path=\"" << path << "\",length="
                  << std::fixed
                  << std::setprecision(LIST_DURATION_PRECISION)
                  << it2->second << "}";

            }
            f << "}\n";
        }

        f << "c.EndList()";
        f.close();
        return true;
    }
    return false;
}

bool UpdateSoundFolder(const std::filesystem::path& path, const int& folder_p, const int& folder_t)
{
    SetGenerationActivityParameters(true, path.filename().string(), folder_p, folder_t);
    bool success = WriteSoundList(ProcessSoundFolder(path), path.filename().string());
    UpdateGenerationActivity(-1, true);
    cout << (success ? " done" : " fail") << endl;
    return success;
}

void ClearFolder(const std::filesystem::path& path)
{
    for (std::filesystem::directory_iterator it(path); it != std::filesystem::directory_iterator(); ++it)
    {
        std::filesystem::remove_all(*it);
    }
}

void UpdateSoundSet(const string& name, const int& folder_p, const int& folder_t)
{
    string list_path = string(LISTPATH) + "/" + name + ".lua";
    std::filesystem::path soundsetpath(string(SOUNDPATH) + "/" + name);

    if (std::filesystem::is_directory(soundsetpath)) // If the directory exists.
    {
        if (!UpdateSoundFolder(soundsetpath, folder_p, folder_t))
        {
            std::filesystem::remove(list_path); // broken sound set
        }
    }
    else if (std::filesystem::is_regular_file(list_path)) // If a related (unneeded/outdated!) sound list exists.
    {
        DisplayGenerationActivity(false, name, folder_p, folder_t);
        std::filesystem::remove(list_path);
        cout << " done" << endl;
    }
}

void UpdateSoundSets(const unordered_map<string, bool>& SoundCacheDiff)
{
    int i = 1;
    for(auto it = SoundCacheDiff.begin(); it != SoundCacheDiff.end(); ++it)
    {
        UpdateSoundSet(it->first, i, SoundCacheDiff.size());
        i++;
    }
}

MissingSoundCacheFiles GetModifiedSoundSets(const SoundCache& cache1, const SoundCache& cache2)
{
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
                                = std::filesystem::last_write_time(GetAbsolutePath(itg->path())).time_since_epoch().count();
                        }
                    }
                }
                else
                {
                    soundcache[its->path().string()]
                        = std::filesystem::last_write_time(GetAbsolutePath(its->path())).time_since_epoch().count();
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



// Cleanup

void CleanupFolder(std::filesystem::path path)
{
    for (std::filesystem::directory_iterator it(path); it != std::filesystem::directory_iterator(); ++it)
    {
        if (is_directory(it->status()))
        {
            std::filesystem::path pp = it->path();

            CleanupFolder(pp);

            if(std::filesystem::is_empty(pp))
            {
                std::filesystem::remove_all(pp);
            }
        }
    }
}


// Modes

int DiffUpdate(const bool &open_ext)
{
    const char * const ln_base   = open_ext ? RXT_LINE : NULL_CHR,
               * const ln_base_e = open_ext ? RND_LINE : NULL_CHR;

    std::chrono::high_resolution_clock::time_point time_begin = std::chrono::high_resolution_clock::now();

    cout << ln_base;
    InitLibAV();

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
        CleanupFolder(SOUNDPATH);
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

    invalid_file_log_close();

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
        ClearFolder(LISTPATH);
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

bool print_topinfo()
{
    getdate();

    const int tag_line_length = string("chatsounds-preprocessor v").length() + 2
        + intDigits(Version::MAJOR)
        + intDigits(Version::MINOR)
        + intDigits(Version::PATCH)
#if HAS_VERSION_DISTRIBUTION
        + strlen(Version::DISTRIBUTION) + 1
#endif
        + string(" by PotcFdk").length() - 1; // -1 for partly overwriting, looks cool

    cout << "        ______            ,                            _   __" << endl
         << "       / ___  |          /|                           | | /  \\\\" << endl
         << "      / /   | |__   __ _| |_ ___  ___  _   _ _ __   __| |/ /\\ \\\\" << endl
         << "     / /    | '_ \\ / _` | __/ __|/ _ \\| | | | `_ \\ / _` |\\ \\\\\\ \\\\" << endl
         << "    ( (     | | | | (_| | |_\\__ \\ (_) | |_| | | | | (_| | \\ \\\\\\//" << endl
         << "     \\ \\    |_| |_|\\__,_|\\______/\\___/ \\__,_|_| |_|\\__,/ \\ \\ \\\\" << endl
         << "      \\ \\____________                                   \\ \\/ //" << endl
         << "       \\____________ \\'\"`-._,-'\"`-._,-'\"`-._,-'\"`-._,-'\"`\\__//" << endl
         << "         ____  | |__) ) __ ___ _ __  _ __ ___   ___ ___  ___ ___  ___  _ __" << endl
         << "        (____) |  ___/ '__/ _ \\ '_ \\| '__/ _ \\ / __/ _ \\/ __/ __|/ _ \\| '__|" << endl
         << "               | |   | | |  __/ |_) | | | (_) | (_(  __/\\__ \\__ \\ (_) | |" << endl
         << "         _____/ /    | |  \\___| .__/|_|  \\___/ \\___\\________/___/\\___/| |" << endl
         << "        (______/     | |      | |                            ^potcfdk | |" << endl
         << "                      \\|      |_|                                     | |" << endl
         << (tag_line_length < 70 ? EXT_LINE : END_LINE)
         << endl;

    if (tag_line_length < 70)
        cout << RXT_LINE;

    cout << "chatsounds-preprocessor v"
         << Version::MAJOR << "."
         << Version::MINOR << "."
         << Version::PATCH
#if HAS_VERSION_DISTRIBUTION
         << Version::DISTRIBUTION
#endif
         << " by PotcFdk" << endl;

    if (tag_line_length < 70)
        cout << EXT_LINE << endl << RXT_LINE;
    else
        cout << endl;

    cout << "Please report any bugs / issues to:" << endl;

    if (tag_line_length < 70)
        cout << RXT_LINE;

    cout << BUGTRACKER_LINK << endl;

    if (tag_line_length < 70)
    {
        cout << EXT_LINE << endl << RXT_LINE;
        return true;
    }
    else
    {
        cout << endl;
        return tag_line_length < 70;
    }
}

void print_versioninfo()
{
    getdate();

    cout << "chatsounds-preprocessor"
         << endl << "Author     : PotcFdk"
         << endl << "Version    : "
         << Version::MAJOR << "."
         << Version::MINOR << "."
         << Version::PATCH
#if HAS_VERSION_DISTRIBUTION
         << Version::DISTRIBUTION
#endif
         << endl << "Build date : "
         << year << "-";
    if (month < 10) cout << 0;
    cout << month << "-";
    if (day < 10) cout << 0;
    cout << day << endl
#if defined(__clang__)
         << "Compiler   : Clang/LLVM, version "
#if defined(__clang_major__) && defined(__clang_minor__) && defined(__clang_patchlevel__)
         << __clang_major__ << '.' << __clang_minor__ << '.' << __clang_patchlevel__
#else
         << __VERSION__
#endif
         << endl;
#elif defined(__GNUG__)
         << "Compiler   : GNU G++, version "
#if defined(__GNUG__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
         << __GNUG__ << '.' << __GNUC_MINOR__ << '.' << __GNUC_PATCHLEVEL__
#else
         << __VERSION__
#endif
         << endl;
#elif defined(_MSC_VER)
         << "Compiler   : Microsoft Visual Studio, version "  << _MSC_VER << endl;
#elif defined(__VERSION__)
         << "Compiler   : Unknown compiler, version " << __VERSION__ << endl;
#else
         << "Compiler   : Unknown compiler" << endl;
#endif

    // More verbose information

    cout << "Boost Info : " << endl
         << " * Boost version " << (BOOST_VERSION / 100000)
         << '.' << ((BOOST_VERSION / 100) % 1000)
         << '.' << (BOOST_VERSION % 100) << endl
         << " * " << BOOST_COMPILER << " on " << BOOST_PLATFORM << endl
         << " * " << BOOST_STDLIB << endl;

    cout << "libav Info : " << endl
         << " * libavformat " << AV_STRINGIFY(LIBAVFORMAT_VERSION) << " (ct:" << LIBAVFORMAT_VERSION_INT << " rt:" << avformat_version() << ")" << endl
         << " * libavcodec  " << AV_STRINGIFY(LIBAVCODEC_VERSION)  << " (ct:" << LIBAVCODEC_VERSION_INT  << " rt:" << avcodec_version()  << ")" << endl
         << " * libavutil   " << AV_STRINGIFY(LIBAVUTIL_VERSION)   << " (ct:" << LIBAVUTIL_VERSION_INT   << " rt:" << avutil_version()   << ")" << endl;
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

    invalid_file_log_close();

    return 0;
}

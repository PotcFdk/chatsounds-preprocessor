/*** You had a choice: looking at this code,  ***
 *** or living in blissful ignorance.         ***
 *** You chose poorly.                        ***/

// Settings

#define BUGTRACKER_LINK "https://github.com/PotcFdk/chatsounds-preprocessor/issues"

#define S_CACHE_VERSION 1
#define S_CACHE_PATH "chatsounds-preprocessor-cache"
#define S_INVALID_FILE_LOG_PATH "invalid-soundfiles.txt"

#define S_LISTPATH "lua/chatsounds/lists_nosend"
#define S_SOUNDPATH "sound/chatsounds/autoadd"
#define S_SOUNDPATH_IGNORELEN 6 // Ignores "sound/"

/// Includes

#include <cstdint>
#include <iostream>
#include <fstream>

#include "version.h"

// List
#include <tuple>
#include <deque>
#include <unordered_map>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

// libbass
#include <bass.h>

using namespace std;


/// Definitions

const uint_fast8_t CACHE_VERSION = S_CACHE_VERSION;
const char * const CACHE_PATH = S_CACHE_PATH;
const char * const INVALID_FILE_LOG_PATH = S_INVALID_FILE_LOG_PATH;

const char * const LISTPATH  = S_LISTPATH;
const char * const SOUNDPATH = S_SOUNDPATH;
const uint_fast8_t SOUNDPATH_IGNORELEN = S_SOUNDPATH_IGNORELEN;

typedef vector<boost::filesystem::path> PathList;

typedef tuple<string, double> SoundInfo;
typedef unordered_map<string, string> SoundMap;
typedef deque<SoundInfo> SoundList;
typedef pair<string, SoundList> NamedSoundList;
typedef deque<NamedSoundList> SoundMasterList;

typedef unordered_map<string, int> SoundCache;
typedef unordered_map<string, bool> MissingSoundCacheFiles;

///

#define DVER(X) #X

#ifdef DISTRIBUTION_VERSION
const char * _DISTRIBUTION_VERSION = DISTRIBUTION_VERSION;
#else
const bool _DISTRIBUTION_VERSION = false;
#endif // DISTRIBUTION_VERSION

const std::vector<unsigned int> valid_samplerates_ogg = {
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

bool cmp_ifspath (const boost::filesystem::path& first, const boost::filesystem::path& second)
{
    return boost::algorithm::ilexicographical_compare(first.c_str(), second.c_str());
}

bool cmp_nsl (const NamedSoundList& a, const NamedSoundList& b)
{
    return boost::algorithm::ilexicographical_compare(a.first.c_str(), b.first.c_str());
}

struct match_char
{
    char c;
    match_char(char c) : c(c) {}
    bool operator()(char x) const { return x == c; }
};

bool bass_init = false;
void InitBass()
{
    if (!bass_init)
    {
        cout << "Initializing BASS Library..." << endl;
        BASS_Init(-1,44100,BASS_DEVICE_FREQ,0,NULL);
        bass_init = true;
    }
}

double GetSoundDuration(const boost::filesystem::path& path, float * freq) // Gets the duration of a sound.
{
    HSTREAM sound = BASS_StreamCreateFile(false, path.c_str(), 0, 0, BASS_STREAM_PRESCAN);
    QWORD bytecount = BASS_ChannelGetLength(sound, BASS_POS_BYTE);
    double length = BASS_ChannelBytes2Seconds(sound, bytecount);
    BASS_ChannelGetAttribute(sound, BASS_ATTRIB_FREQ, freq);
    BASS_StreamFree(sound);
    return length;
}

boost::filesystem::path GetAbsolutePath(const boost::filesystem::path& path)
{
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
        boost::filesystem::path ret ("\\\\?\\");
        ret += boost::filesystem::absolute(path);
        ret.make_preferred();
        return ret;
    #else
        return boost::filesystem::absolute(path);
    #endif
}

boost::optional<SoundInfo> GetSoundInfo(const boost::filesystem::path& path) // Assembles an infolist about a sound.
{
    if (path.has_extension())
    {
        string ext = path.extension().string();
        boost::algorithm::to_lower(ext);

        if (ext == ".ogg" || ext == ".mp3" || ext == ".wav")
        {
            string s_path = path.generic_string();
            boost::filesystem::path full_path = GetAbsolutePath(path);

            float freq = 0;
            double duration = GetSoundDuration(full_path, &freq);
            if (
                ( ext != ".ogg"
                    || (std::find(valid_samplerates_ogg.begin(), valid_samplerates_ogg.end(), freq)
                        != valid_samplerates_ogg.end()) )
               )
            {
                boost::algorithm::erase_head(s_path, SOUNDPATH_IGNORELEN);
                return SoundInfo(s_path, duration);
            }
            else
            {
                cout << "[invalid sample rate] " << s_path << ": " << freq << endl;
            }
        }
    }
    return false;
}

NamedSoundList ProcessSoundGroup(const boost::filesystem::path& path)
{
    SoundList list;

    PathList paths;
    copy(boost::filesystem::directory_iterator(path), boost::filesystem::directory_iterator(), back_inserter(paths));
    sort(paths.begin(), paths.end(), cmp_ifspath); // To make sure it's sorted.

    for(PathList::const_iterator it (paths.begin()); it != paths.end(); ++it)
    {
        boost::filesystem::path sub_path = GetAbsolutePath((*it));

        if (boost::filesystem::is_regular_file(sub_path))
        {
            if (boost::optional<SoundInfo> soundinfo = GetSoundInfo(*it))
            {
                list.push_back(*soundinfo);
            }
            else
            {
                invalid_file_log_open();
                invalid_file_log << it->generic_string() << endl;
            }
        }
    }
    NamedSoundList nlist(boost::algorithm::to_lower_copy(path.filename().string()), list);
    list.clear();
    list.shrink_to_fit();
    return nlist;
}

SoundMap ParseSoundMap(const boost::filesystem::path& path)
{
    SoundMap soundmap;

    std::ifstream f(path.string());

    if (!f.fail())
    {
        std::string ln, source, destination;
        while (std::getline(f, ln))
        {
            boost::algorithm::trim(ln);

            if (!boost::algorithm::starts_with(ln, "#"))
            {
                std::vector<std::string> items;
                boost::algorithm::split(items, ln, match_char(';'));
                if (items.size() == 2) // source and destination exist
                {
                    source      = boost::algorithm::trim_copy(items.at(0));
                    destination = boost::algorithm::trim_copy(items.at(1));
                    soundmap[source] = destination;
                }
            }
        }
        f.close();
    }

    return soundmap;
}

SoundMasterList ProcessSounds(const boost::filesystem::path& path) // Scans a subdirectory and compiles all the soundinfos into a list.
{
    SoundMasterList list;
    SoundMap soundmap;

    PathList paths;
    copy(boost::filesystem::directory_iterator(path), boost::filesystem::directory_iterator(), back_inserter(paths));
    sort(paths.begin(), paths.end(), cmp_ifspath);

    for(PathList::const_iterator it (paths.begin()); it != paths.end(); ++it)
    {
        boost::filesystem::path sub_path = GetAbsolutePath(*it);

        if (is_directory(sub_path)) // It's a sound group.
        {
            list.push_back(ProcessSoundGroup(*it));
        }
        else if (boost::filesystem::is_regular_file(sub_path)) // It's a single file.
        {
            if (boost::iequals(sub_path.filename().string(), "map.txt"))
            {
                soundmap = ParseSoundMap(sub_path);
            }
            else
            {
                if (boost::optional<SoundInfo> soundinfo = GetSoundInfo(*it))
                {
                    SoundList sl;
                    sl.push_back(*soundinfo);
                    list.push_back(
                        NamedSoundList(boost::algorithm::to_lower_copy(it->filename().replace_extension("").string()),
                            sl));
                    sl.clear();
                    sl.shrink_to_fit();
                }
                else
                {
                    invalid_file_log_open();
                    invalid_file_log << it->generic_string() << endl;
                }
            }
        }
    }

    if (soundmap.size() > 0) // we have a custom sound map
    {
        for (SoundMasterList::iterator it = list.begin(); it != list.end(); ++it)
        {
            for (SoundMap::iterator it2 = soundmap.begin(); it2 != soundmap.end(); ++it2)
            {
                if (boost::iequals(it->first, it2->second))
                {
                    list.push_back(NamedSoundList(it2->first, it->second)); // trust me, it2->first and it->second, this makes sense
                }
            }
        }
    }

    sort(list.begin(), list.end(), cmp_nsl);

    return list;
}

SoundMasterList ProcessSoundFolder(const boost::filesystem::path& path)
{
    if (is_directory(path))
    {
        SoundMasterList list = ProcessSounds(path);
        return list;
    }
    return SoundMasterList();
}

bool WriteSoundList(const SoundMasterList& list, const string& listname)
{
    bool first_in_multientry;
    string soundlist = "c.StartList(\"" + listname + "\")\n";

    for (SoundMasterList::const_iterator it=list.begin() ; it < list.end(); it++ )
    {
        first_in_multientry = true;
        SoundList sndlist = get<1>(*it);
        soundlist += "L[\"" + get<0>(*it) + "\"]={";
        for (SoundList::const_iterator it2=sndlist.begin() ; it2 < sndlist.end(); it2++ )
        {
            SoundInfo sndinfo = *it2;
            soundlist += (first_in_multientry ? "{path=\"" : ",{path=\"")
                + get<0>(sndinfo) + "\",length=" + boost::lexical_cast<std::string>(get<1>(sndinfo)) + "}";
            first_in_multientry = false;
        }
        soundlist += "}\n";
    }

    soundlist += "c.EndList()";

    std::ofstream f(string(LISTPATH) + "/" + listname + ".lua", std::ofstream::binary);
    if (!f.fail())
    {
        f << soundlist;
        f.close();
        return true;
    }
    return false;
}

void UpdateSoundFolder(const boost::filesystem::path& path, const int& folder_p, const int& folder_t)
{
#define P_LENGTH 57

    string p1 = '"' + path.filename().string() + '"' + " (" + boost::lexical_cast<string>(folder_p) + '/' + boost::lexical_cast<string>(folder_t) + ") ...";
    p1 = p1.size() >= P_LENGTH ? "..." + p1.substr(p1.size() - P_LENGTH + 3) : p1 + string(P_LENGTH - p1.size(), ' ');

    cout << "Generating list: " << p1 << " ";

    SoundMasterList list = ProcessSoundFolder(path);
    BuildSoundList(list, path.filename().string());

    cout << "done" << endl;
}

void ProcessSoundFolders(const boost::filesystem::path& path)
{
    const int d_count = std::count_if(
                            boost::filesystem::directory_iterator(path),
                            boost::filesystem::directory_iterator(),
                            boost::bind( static_cast<bool(*)(const boost::filesystem::path & path)>(boost::filesystem::is_directory), boost::bind( &boost::filesystem::directory_entry::path, _1 ) ));

    int d_i = 1;

    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if ( is_directory(it->status()) )
        {
            UpdateSoundFolder(it->path(), d_i, d_count);
            d_i++;
        }
    }
}

void ClearFolder(const boost::filesystem::path& path)
{
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        boost::filesystem::remove_all(*it);
    }
}

void UpdateSoundSet(const string& name, const int& folder_p, const int& folder_t)
{
    string list_path = string(LISTPATH) + "/" + name + ".lua";
    boost::filesystem::path soundsetpath(string(SOUNDPATH) + "/" + name);

    if (boost::filesystem::is_directory(soundsetpath))
    {
        UpdateSoundFolder(soundsetpath, folder_p, folder_t);
    }
    else if(boost::filesystem::is_regular_file(list_path))
    {
#define P2_LENGTH 59
        string p1 = '"' + name + '"' + " (" + boost::lexical_cast<string>(folder_p) + '/' + boost::lexical_cast<string>(folder_t) + ") ...";
        p1 = p1.size() >= P2_LENGTH ? "..." + p1.substr(p1.size() - P2_LENGTH + 3) : p1 + string(P2_LENGTH - p1.size(), ' ');

        cout << "Deleting list: " << p1 << " ";
        boost::filesystem::remove(list_path);
        cout << "done" << endl;
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

    // Add differences to list:

    for ( auto it = cache1.begin(); it != cache1.end(); ++it )
    {
        if (cache2[it->first] != it->second)
        {
            boost::filesystem::path path(it->first);

            int i = 0;

            while (path.parent_path().filename() != "autoadd")
            {
                path = path.parent_path();

                if (i > 300)
                    throw 31;
                else
                    i++;
            }

            list[path.filename().generic_string()] = true;
        }
    }

    for ( auto it = cache2.begin(); it != cache2.end(); ++it )
    {
        if (cache1[it->first] != it->second)
        {
            boost::filesystem::path path(it->first);

            int i = 0;

            while (path.parent_path().filename() != "autoadd")
            {
                path = path.parent_path();

                if (i > 300)
                    throw 32;
                else
                    i++;
            }

            list[path.filename().generic_string()] = true;
        }
    }

    return list;
}

void AddMissingLists(MissingSoundCacheFiles& list, const SoundCache& soundcache)
{
    for ( auto it = soundcache.begin(); it != soundcache.end(); ++it )
    {
        boost::filesystem::path path(it->first);

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
        if (!boost::filesystem::is_regular_file(list_path))
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
        if (boost::filesystem::exists(CACHE_PATH))
            boost::filesystem::remove(CACHE_PATH);
    }
    catch (boost::archive::archive_exception e)
    {
        cout << "Boost exception: " << e.what() << endl;
    }
}

SoundCache ReadSoundCache()
{
    if (!boost::filesystem::exists(CACHE_PATH))
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

    for (boost::filesystem::directory_iterator it(SOUNDPATH); it != boost::filesystem::directory_iterator(); ++it)
    {
        if (is_directory(it->status()))
        {
            for (boost::filesystem::directory_iterator its(it->path()); its != boost::filesystem::directory_iterator(); ++its)
            {
                if (is_directory(its->status()))
                {
                    for (boost::filesystem::directory_iterator itg(its->path()); itg != boost::filesystem::directory_iterator(); ++itg)
                    {
                        if (boost::filesystem::is_regular_file(itg->status()))
                        {
                            soundcache[itg->path().generic_string()]
                                    = boost::filesystem::last_write_time(GetAbsolutePath(itg->path()));
                        }
                    }
                }
                else
                {
                    soundcache[its->path().generic_string()]
                            = boost::filesystem::last_write_time(GetAbsolutePath(its->path()));
                }
            }
        }
    }

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

void CleanupFolder(boost::filesystem::path path)
{
    for (boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if (is_directory(it->status()))
        {
            boost::filesystem::path pp = it->path();

            CleanupFolder(pp);

            if(boost::filesystem::is_empty(pp))
            {
                boost::filesystem::remove_all(pp);
            }
        }
    }
}


// Modes

int DiffUpdate()
{
    InitBass();

    cout << "Resetting invalid soundfile log..." << endl;

    try
    {
        boost::filesystem::remove(INVALID_FILE_LOG_PATH);
    }
    catch (boost::filesystem::filesystem_error e)
    {
        cout << "Cannot reset invalid soundfile log: " << endl << "  " << e.what() << endl << endl;
    }

    cout << "Cleaning up sound folder..." << endl;
    try
    {
        CleanupFolder(SOUNDPATH);
    }
    catch(boost::filesystem::filesystem_error e)
    {
        cout << "Boost exception: " << e.what() << endl;
        throw 1;
    }

    SoundCache soundcache;
    try
    {
        soundcache = ReadSoundCache();
    }
    catch (boost::archive::archive_exception e)
    {
        cout << "Boost exception: " << e.what() << endl;
        EraseSoundCache();
    }
    catch (int e)
    {
        if (e == 11) {} // No cache file.
        else if (e == 12)
            cout << "Incompatible Cache, reset." << endl;
        else
            cout << "ERROR " << e << endl;
        EraseSoundCache();
    }

    cout << endl << "Scanning sounds...";

    SoundCache new_soundcache;
    MissingSoundCacheFiles to_be_updated;

    try
    {
        new_soundcache = GenerateSoundCache();
        to_be_updated = GetModifiedSoundSets(soundcache, new_soundcache);
        AddMissingLists(to_be_updated, new_soundcache);
    }
    catch (boost::filesystem::filesystem_error e)
    {
        cout << "  ERR" << endl
            << "Boost exception: " << e.what() << endl;
        throw 60;
    }

    cout << "  OK" << endl;

    UpdateSoundSets(to_be_updated);
    WriteSoundCache(new_soundcache);

    cout << "List generation has finished." << endl;

    invalid_file_log_close();

    if (boost::filesystem::exists(INVALID_FILE_LOG_PATH))
    {
        cout << endl << "[Information] "
            "Some invalid files were found during the generation." << endl
            << "Please open '" << INVALID_FILE_LOG_PATH << "' and double-check these files." << endl
            << "They might be corrupt, empty or have an unsupported sample rate." << endl
            << "If you have confirmed they work in-game and believe this is an error, visit" << endl
            << "  " << BUGTRACKER_LINK << endl << "and post a bug report." << endl
            << "Press ENTER to exit..." << endl;
        cin.get();
    }

    return 0;
}

int FullUpdate()
{
    cout << "Resetting cache..." << endl;

    try
    {
        boost::filesystem::remove(CACHE_PATH);
    }
    catch (boost::filesystem::filesystem_error e)
    {
        cout << "Cannot reset cache: " << endl << "  " << e.what() << endl << endl;
    }

    cout << "Deleting old lists..." << endl;
    try
    {
        ClearFolder(LISTPATH);
    }
    catch(boost::filesystem::filesystem_error e)
    {
        cout << "Boost exception: " << e.what() << endl;
        throw 3;
    }

    return DiffUpdate();
}



void showError(int e)
{
    cout << endl << "Preprocessor Exception: ERROR " << e << endl;

    if (e >= 30)
        cout << "Please report this bug at:" << endl << "  " << BUGTRACKER_LINK << endl;
    else
        cout << "The error message above should give you an idea on what might be wrong." << endl;

    cout << endl << "Press ENTER to exit..." << endl;
    cin.get();
}



void print_topinfo()
{
    cout << "chatsounds-preprocessor v"
         << AutoVersion::MAJOR << "."
         << AutoVersion::MINOR << "."
         << AutoVersion::BUILD;

    if (_DISTRIBUTION_VERSION)
        cout << "-" << _DISTRIBUTION_VERSION;

    cout << " by PotcFdk  (Build "
         << AutoVersion::BUILDS_COUNT << " @ "
         << AutoVersion::YEAR << "/"
         << AutoVersion::MONTH << "/"
         << AutoVersion::DATE << ")" << endl << endl

         << "Please report any bugs / issues to:" << endl
         << BUGTRACKER_LINK << endl << endl;
}

void print_versioninfo()
{
    cout << "chatsounds-preprocessor"
         << endl << "Version    : "
         << AutoVersion::MAJOR << "."
         << AutoVersion::MINOR << "."
         << AutoVersion::BUILD;

    if (_DISTRIBUTION_VERSION)
        cout << "-" << _DISTRIBUTION_VERSION;

    cout << endl << "Build      : "
         << AutoVersion::BUILDS_COUNT
         << endl << "Build date : "
         << AutoVersion::YEAR << "/"
         << AutoVersion::MONTH << "/"
         << AutoVersion::DATE
         << endl

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
}



int Launch_DiffUpdate()
{
    cout << "Running in DIFF mode." << endl;

    try
    {
        DiffUpdate();
    }
    catch (int e)
    {
        showError(e);
        return -1;
    }
    catch (...)
    {
        showError(99);
    }
    return 0;
}

int Launch_FullUpdate()
{
    cout << "Running in FULL mode." << endl;

    try
    {
        FullUpdate();
    }
    catch (int e)
    {
        showError(e);
        return -1;
    }
    catch (...)
    {
        showError(99);
    }
    return 0;
}





int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        print_topinfo();
        return Launch_DiffUpdate();
    }
    else if (argc >= 2)
    {
        string clp(argv[1]);
        boost::algorithm::to_lower(clp);

        if (clp == "-v" || clp == "--version")
            print_versioninfo();
        else
        {
            print_topinfo();

            if (clp == "-f" || clp == "--full")
                return Launch_FullUpdate();
            else if (clp == "-l" || clp == "--lite" || clp == "-d" || clp == "-diff")
                return Launch_DiffUpdate();
            else if (clp == "-h" || clp == "--help")
            {
                cout << "Usage: " << endl
                     << " -f | --full     -  Full, uncached list generation" << endl
                     << " -d | --diff     -  Normal, cached list generation (default)" << endl
                     << " -l | --lite     -  Same as --diff" << endl
                     << " -h | --help     -  Usage help (this text right here)" << endl
                     << " -v | --version  -  Show the program version" << endl;
            }
            else
                cout << "Unknown command line parameter: " << clp << endl
                     << "For usage help, see -h or --help." << endl;
        }
    }

    invalid_file_log_close();

    return 0;
}

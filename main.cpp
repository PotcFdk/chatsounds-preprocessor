// Settings

#define LISTPATH "lua/chatsounds/lists_nosend"
#define SOUNDPATH "sound/chatsounds/autoadd"
#define SOUNDPATH_IGNORELEN 6 // Ignores "sound/"


/// Includes

#include <iostream>

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


/// Defines

typedef tuple<string, double> SoundInfo;
typedef deque<SoundInfo> SoundList;
typedef pair<string, SoundList> NamedSoundList;
typedef deque<NamedSoundList> SoundMasterList;

typedef unordered_map<string, int> SoundCache;

#define CACHE_VERSION 1
#define CACHE_PATH "chatsounds-preprocessor-cache"


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

double GetSoundDuration(boost::filesystem::path path) // Gets the duration of a sound.
{
    HSTREAM sound = BASS_StreamCreateFile(false, path.c_str(), 0, 0, BASS_STREAM_PRESCAN);
    QWORD bytecount = BASS_ChannelGetLength(sound, BASS_POS_BYTE);
    double length = BASS_ChannelBytes2Seconds(sound, bytecount);
    BASS_StreamFree(sound);
    return length;
}

SoundInfo GetSoundInfo(boost::filesystem::path path) // Assembles an infolist about a sound.
{
    if (path.has_extension())
    {
        string ext = path.extension().string();
        boost::algorithm::to_lower(ext);

        if (ext == ".ogg" || ext == ".mp3" || ext == ".wav")
        {
            string s_path = path.generic_string();
            double duration = GetSoundDuration(s_path);
            boost::algorithm::erase_head(s_path, SOUNDPATH_IGNORELEN);
            return SoundInfo(s_path, duration);
        }
    }
    return SoundInfo("",0);
}

NamedSoundList ProcessSoundGroup(boost::filesystem::path path)
{
    SoundList list;
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if (boost::filesystem::is_regular_file(it->status()))
        {
            list.push_back(GetSoundInfo(it->path()));
        }
    }
    NamedSoundList nlist(path.filename().string(), list);
    list.clear();
    list.shrink_to_fit();
    return nlist;
}

SoundMasterList ProcessSounds(boost::filesystem::path path) // Scans a subdirectory and compiles all the soundinfos into a list.
{
    SoundMasterList list;
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if ( is_directory(it->status()) )
        {
            list.push_back(ProcessSoundGroup(it->path()));
        }
        else if ( boost::filesystem::is_regular_file(it->status()) )
        {
            SoundInfo soundinfo = GetSoundInfo(it->path());
            if (get<1>(soundinfo) > 0)
            {
                SoundList sl;
                sl.push_back(soundinfo);
                list.push_back(NamedSoundList(it->path().filename().replace_extension("").string(),sl));
                sl.clear();
                sl.shrink_to_fit();
            }
        }
    }
    return list;
}

SoundMasterList ProcessSoundFolder(boost::filesystem::path path)
{
    if (is_directory(path))
    {
        SoundMasterList list = ProcessSounds(path);
        return list;
    }
    return SoundMasterList();
}

void BuildSoundList(SoundMasterList list, string listname)
{
    string soundlist = "c.StartList(\"" + listname + "\")\n";

    for(SoundMasterList::const_iterator it=list.begin() ; it < list.end(); it++ )
    {
        SoundList sndlist = get<1>(*it);
        soundlist += "L[\"" + get<0>(*it) + "\"]={";
        for(SoundList::const_iterator it2=sndlist.begin() ; it2 < sndlist.end(); it2++ )
        {
            SoundInfo sndinfo = *it2;
            soundlist += "{path=\"" + get<0>(sndinfo) + "\",length=" + boost::lexical_cast<std::string>(get<1>(sndinfo)) + "},";
        }
        soundlist += "}\n";
    }

    soundlist += "c.EndList()";

    std::ofstream f( string(LISTPATH) + "/" + listname + ".lua" );
    if ( !f.fail() )
    {
        f << soundlist;
        f.close();
    }
}

void UpdateSoundFolder(boost::filesystem::path path, int folder_p, int folder_t)
{
    #define P_LENGTH 57

    string p1 = '"' + path.filename().string() + '"' + " (" + boost::lexical_cast<string>(folder_p) + '/' + boost::lexical_cast<string>(folder_t) + ") ...";
    p1 = p1.size() >= P_LENGTH ? "..." + p1.substr(p1.size() - P_LENGTH + 3) : p1 + string(P_LENGTH - p1.size(), ' ');

    cout << "Generating list: " << p1 << " ";

    SoundMasterList list = ProcessSoundFolder(path);
    BuildSoundList(list, path.filename().string());

    cout << "done" << endl;
}

void ProcessSoundFolders(boost::filesystem::path path)
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

void ClearFolder(boost::filesystem::path path)
{
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        boost::filesystem::remove_all(*it);
    }
}

void UpdateSoundSet(string name, int folder_p, int folder_t)
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

void UpdateSoundSets(unordered_map<string, bool> SoundCacheDiff)
{
    int i = 1;
    for(auto it = SoundCacheDiff.begin(); it != SoundCacheDiff.end(); ++it)
    {
        UpdateSoundSet(it->first, i, SoundCacheDiff.size());
        i++;
    }
}

SoundCache GetSoundCache()
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
        boost::filesystem::remove(CACHE_PATH);
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

    for(boost::filesystem::directory_iterator it(SOUNDPATH); it != boost::filesystem::directory_iterator(); ++it)
    {
        if ( is_directory(it->status()) )
        {
            for(boost::filesystem::directory_iterator its(it->path()); its != boost::filesystem::directory_iterator(); ++its)
            {
                if ( is_directory(its->status()) )
                {
                    for(boost::filesystem::directory_iterator itg(its->path()); itg != boost::filesystem::directory_iterator(); ++itg)
                    {
                        if ( boost::filesystem::is_regular_file(itg->status()) )
                        {
                            soundcache[ itg->path().generic_string() ] = boost::filesystem::last_write_time( itg->path() );
                        }
                    }
                }
                else
                {
                    soundcache[ its->path().generic_string() ] = boost::filesystem::last_write_time( its->path() );
                }
            }
        }
    }

    return soundcache;
}



unordered_map<string, bool> GetModifiedSoundSets(SoundCache cache1, SoundCache cache2)
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

void AddMissingLists(unordered_map<string, bool> * list, SoundCache soundcache)
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
            (*list)[name] = true;
        }
    }
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

            if( boost::filesystem::is_empty( pp ))
            {
               boost::filesystem::remove_all(pp);
            }
        }
    }
}


// Modes

int Main_DiffUpdate()
{
    cout << "Running in DIFF mode." << endl;

    InitBass();

    cout << "Cleaning up sound folder..." << endl;
    CleanupFolder(SOUNDPATH);

    SoundCache soundcache;
    try
    {
        soundcache = GetSoundCache();
    }
    catch (boost::archive::archive_exception e)
    {
        cout << "Boost exception: " << e.what() << endl;
    }
    catch (int e)
    {
        if (e == 11) {} // No cache file.
        else if (e == 12)
            cout << "Incompatible Cache, reset." << endl;
        else
            cout << "ERROR " << e << endl;
    }

    cout << endl;

    try
    {
        SoundCache new_soundcache = GenerateSoundCache();
        unordered_map<string, bool> SoundCacheDiff = GetModifiedSoundSets(soundcache, new_soundcache);
        AddMissingLists(&SoundCacheDiff, new_soundcache);
        UpdateSoundSets(SoundCacheDiff);
        WriteSoundCache(new_soundcache);
    }
    catch (int e)
    {
        cout << "ERROR: " << e << endl;
        return -e;
    }

    cout << "List generation has finished." << endl;
    return 0;
}

int Main_FullUpdate()
{
    cout << "Running in FULL mode." << endl;

    InitBass();
    cout << "Resetting cache..." << endl;

    try { boost::filesystem::remove(CACHE_PATH); }
        catch (boost::filesystem::filesystem_error e)
            { cout << "Cannot reset cache: " << endl << "  " << e.what() << endl << endl; }

    cout << "Deleting old lists..." << endl;
    ClearFolder(LISTPATH);

    cout << "Cleaning up sound folder..." << endl;
    CleanupFolder(SOUNDPATH);

    cout << endl;
    ProcessSoundFolders(boost::filesystem::path(SOUNDPATH));

    cout << "List generation has finished." << endl;
    return 0;
}









int main(int argc, char* argv[])
{
    cout << "chatsounds-preprocessor v" +
         boost::lexical_cast<std::string>(AutoVersion::MAJOR) + "." +
         boost::lexical_cast<std::string>(AutoVersion::MINOR) + "." +
         boost::lexical_cast<std::string>(AutoVersion::BUILD) + " by PotcFdk  (Build " +
         boost::lexical_cast<std::string>(AutoVersion::BUILDS_COUNT) + ")" << endl << endl;

    if (argc == 1)
        return Main_DiffUpdate();
    else if (argc >= 2)
    {
        string clp(argv[1]);
        if (clp == "-f" || clp == "--full")
            return Main_FullUpdate();
        else if (clp == "-l" || clp == "--lite" || clp == "-d" || clp == "-diff")
            return Main_DiffUpdate();
        else if (clp == "-h" || clp == "--help")
        {
            cout << "Usage: " << endl
                << " -f | --full  -  Full, uncached list generation" << endl
                << " -l | --lite  -  Normal, cached list generation" << endl
                << " -d | --diff  -  Same as --lite" << endl
                << " -h | --help  -  Usage help (this text right here)" << endl;
        }
        else
            cout << "Unknown command line parameter: " << clp << endl
                << "For usage help, see -h or --help." << endl;
    }

    return 0;
}

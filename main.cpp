#include <iostream>

#include "version.h"

// List
#include <tuple>
#include <deque>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

// libbass
#include <bass.h>

using namespace std;

// Defines

typedef tuple<string, double> SoundInfo;
typedef deque<SoundInfo> SoundList;
typedef pair<string, SoundList> NamedSoundList;
typedef deque<NamedSoundList> SoundMasterList;


#define LISTPATH "lua/chatsounds/lists_nosend"
#define SOUNDPATH "sound/chatsounds/autoadd"
#define SOUNDPATH_IGNORELEN 6 // Ignores "sound/"



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
            #define P_LENGTH 57

            string p1 = '"' + it->path().filename().string() + '"' + " (" + boost::lexical_cast<string>(d_i) + '/' + boost::lexical_cast<string>(d_count) + ") ...";
            p1 = p1.size() >= P_LENGTH ? "..." + p1.substr(p1.size() - P_LENGTH + 3) : p1 + string(P_LENGTH - p1.size(), ' ');

            cout << "Generating list: " << p1;

            SoundMasterList list = ProcessSoundFolder(it->path());
            BuildSoundList(list, it->path().filename().string());

            cout << " done" << endl;
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

int main()
{
    cout << "chatsounds-preprocessor v" +
            boost::lexical_cast<std::string>(AutoVersion::MAJOR) + "." +
            boost::lexical_cast<std::string>(AutoVersion::MINOR) + "." +
            boost::lexical_cast<std::string>(AutoVersion::BUILD) + " by PotcFdk  (Build " +
            boost::lexical_cast<std::string>(AutoVersion::BUILDS_COUNT) + ")" << endl << endl;

    cout << "Deleting old lists..." << endl;
    ClearFolder(LISTPATH);

    cout << "Initializing BASS Library..." << endl;
    BASS_Init(-1,44100,BASS_DEVICE_FREQ,0,NULL);

    cout << endl;
    ProcessSoundFolders(boost::filesystem::path(SOUNDPATH));

    cout << "List generation has finished." << endl;

    return 0;
}

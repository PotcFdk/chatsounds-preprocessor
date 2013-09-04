#include <iostream>

// List
#include <tuple>
#include <deque>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

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



double GetSoundDuration(boost::filesystem::path path) // Gets the duration of a sound.
{
    HSTREAM sound = BASS_StreamCreateFile(false, path.c_str(), 0, 0, BASS_STREAM_PRESCAN);
    QWORD bytecount = BASS_ChannelGetLength(sound, BASS_POS_BYTE);
    return BASS_ChannelBytes2Seconds(sound, bytecount);
}

SoundInfo GetSoundInfo(boost::filesystem::path path) // Assembles an infolist about a sound.
{
    if (path.has_extension())
    {
        string ext = path.extension().string();
        boost::algorithm::to_lower(ext);

        if (ext == ".ogg" || ext == ".mp3" || ext == ".wav")
        {
            return SoundInfo(path.generic_string(),GetSoundDuration(path));
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
    return nlist;
}

void ProcessSounds(boost::filesystem::path path, SoundMasterList * list) // Scans a subdirectory and compiles all the soundinfos into a list.
{
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if ( is_directory(it->status()) )
        {
            list->push_back(ProcessSoundGroup(it->path()));
        }
        else if ( boost::filesystem::is_regular_file(it->status()) )
        {
            SoundInfo soundinfo = GetSoundInfo(it->path());
            if (get<1>(soundinfo) > 0)
            {
                SoundList sl;
                sl.push_back(soundinfo);
                list->push_back(NamedSoundList(it->path().filename().replace_extension("").string(),sl));
            }
        }
    }
}

SoundMasterList ProcessSoundFolder(boost::filesystem::path path)
{
    SoundMasterList list;

    if (is_directory(path))
    {
        ProcessSounds(path, &list);
    }

    return list;
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
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if ( is_directory(it->status()) )
        {
            SoundMasterList list = ProcessSoundFolder(it->path());
            BuildSoundList(list, it->path().filename().string());
            cout << "Generated List: " << it->path().filename() << endl;
        }
    }
}



int main()
{
    BASS_Init(-1,44100,BASS_DEVICE_FREQ,0,NULL);

    ProcessSoundFolders(boost::filesystem::path(SOUNDPATH));
    return 0;
}

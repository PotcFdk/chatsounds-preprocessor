#include <iostream>

// List
#include <tuple>
#include <deque>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

// libbass
#include <bass.h>

using namespace std;

// Defines

typedef tuple<string, string, double> SoundInfo;
typedef deque<SoundInfo> SoundList;





double GetSoundDuration(boost::filesystem::path path)
{
    HSTREAM sound = BASS_StreamCreateFile(false, path.c_str(), 0, 0, BASS_STREAM_PRESCAN);
    QWORD bytecount = BASS_ChannelGetLength(sound, BASS_POS_BYTE);
    return BASS_ChannelBytes2Seconds(sound, bytecount);
}

SoundInfo GetSoundInfo(boost::filesystem::path path, bool soundgroup)
{
    if (path.has_extension())
    {
        string ext = path.extension().string();
        boost::algorithm::to_lower(ext);

        if (ext == ".ogg" || ext == ".mp3" || ext == ".wav")
        {
            string name;
            if (soundgroup)
                name = path.parent_path().filename().string();
            else
                name = path.filename().replace_extension("").string();

            double duration = GetSoundDuration(path);

            return SoundInfo(path.string(), name, duration);
        }
    }
    return SoundInfo("","",0);
}

void ProcessSounds(boost::filesystem::path path, bool soundgroup, SoundList * list)
{
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if ( is_directory(it->status()) )
        {
            ProcessSounds(it->path(), true, list);
        }
        else
        {
            list->push_back(
                GetSoundInfo(it->path(), soundgroup)
            );
        }
    }
}

void ProcessSoundFolders(boost::filesystem::path path)
{
    SoundList list;
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if ( is_directory(it->status()) )
        {
            ProcessSounds(it->path(), false, &list);
        }
    }

    for(SoundList::const_iterator it=list.begin() ; it < list.end(); it++ )
    {
        SoundInfo sndinfo = *it;
        cout << get<1>(sndinfo) << "\tDuration: " << get<2>(sndinfo) << endl;
    }
}


int main()
{
    BASS_Init(-1,44100,BASS_DEVICE_FREQ,0,NULL);

    ProcessSoundFolders(boost::filesystem::path("sound/chatsounds/autoadd"));
    return 0;
}

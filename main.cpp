#include <iostream>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

using namespace std;

void GetSoundInfo(boost::filesystem::path path, bool soundgroup)
{
    if (!path.has_extension())
        return;

    string ext = path.extension().string();
    boost::algorithm::to_lower(ext);

    if (ext == ".ogg" || ext == ".mp3" || ext == ".wav")
    {

    }
}

void ProcessSounds(boost::filesystem::path path, bool soundgroup)
{
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if ( is_directory(it->status()) )
        {
            ProcessSounds(it->path(), true);
        }
        else
        {
            GetSoundInfo(it->path(), soundgroup);
        }
    }
}

void ProcessSoundFolders(boost::filesystem::path path)
{
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if ( is_directory(it->status()) )
        {
            ProcessSounds(it->path(), false);
        }
    }
}


int main()
{
    ProcessSoundFolders(boost::filesystem::path("sound/chatsounds/autoadd"));
    return 0;
}

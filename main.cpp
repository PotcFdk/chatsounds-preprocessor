#include <iostream>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

using namespace std;

void GetSoundInfo(boost::filesystem::path path)
{

}

void ProcessSounds(boost::filesystem::path path)
{
    for(boost::filesystem::directory_iterator it(path); it != boost::filesystem::directory_iterator(); ++it)
    {
        if ( is_directory(it->status()) )
        {
            boost::filesystem::path pp = it->path();
        }
    }
}


int main()
{
    ProcessSounds(boost::filesystem::path("sound/chatsounds/autoadd"));
    return 0;
}

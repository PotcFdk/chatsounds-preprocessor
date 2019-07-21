#include "constants.hpp"
#include "preprocessor.hpp"
#include "util.hpp"
#include <iostream>
#include <string>
#include <fstream>

// libavformat
extern "C" {
#include <libavformat/avformat.h>
}

using std::get;

bool Preprocessor::avformat_init = false;

void Preprocessor::InitLibAV() {
    if (!avformat_init) {
        std::cout << "Initializing libavformat..." << std::endl;
        av_log_set_level(AV_LOG_ERROR);
        avformat_init = true;
    }
}

Preprocessor::Preprocessor (std::filesystem::path p) : path (p) {
    InitLibAV();
}


SoundInfoMap Preprocessor::ProcessSoundSet (const std::filesystem::path& path) // Scans a subdirectory and compiles all the soundinfos into a list.
{
    SoundInfoMap list;
    AliasMap aliasmap;

    PathList paths;
    copy(std::filesystem::directory_iterator(path), std::filesystem::directory_iterator(), back_inserter(paths));

    int i = 1, total = paths.size();

    for(PathList::const_iterator it (paths.begin()); it != paths.end(); ++it, ++i)
    {
        std::filesystem::path sub_path = std::filesystem::absolute (*it);

        //UpdateGenerationActivity(100 * i / total); //TODO

        if (is_directory(sub_path)) // It's a sound group.
        {
            SoundFileInfoList info = ProcessSoundGroup(*it);
            SoundName _info_list_name (boost::algorithm::to_lower_copy (it->filename().string()));
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
                std::ifstream _sub_path (sub_path);
                aliasmap = ParseAliasMap (_sub_path);
            }
            else
            {
                if (std::optional<SoundFileInfo> soundinfo = GetSoundFileInfo (*it))
                {
                    SoundFileInfo info = *soundinfo;
                    SoundName _info_list_name (boost::algorithm::to_lower_copy (it->filename().replace_extension("").string()));
                    if (!list.count (_info_list_name))
                    { // Doesn't exist, just add it.
                        list[_info_list_name] = SoundFileInfoList {info};
                    }
                    else
                    { // Exists, add it.
                        list[_info_list_name].push_back(info);
                    }
                }
            }
        }
    }

    if (aliasmap.size() > 0) // we have a custom sound map
    {
        for (AliasMap::iterator it = aliasmap.begin(); it != aliasmap.end(); ++it) // iterate over the sound map
        {
            // Remember: *it = [ ( source, alias, (bool) replace ) ]
            if (list.count(get<0>(*it))) // if source exists in the list
            {
                SoundName alias (get<1>(*it));

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
                            std::back_insert_iterator<SoundFileInfoList>(list[alias]));
                    }
                }
            } // end if source exists in the list
        } // end iterate over the sound map
    } // end we have a custom sound map

    // Sort SoundInfo elements inside the SoundInfos in our SoundInfoMap.
    for (SoundInfoMap::iterator it = list.begin(); it != list.end(); ++it) {
        it->second.sort(cmp_sfi);
    }

    return list;
}

bool Preprocessor::UpdateSoundSet(const std::filesystem::path& path, const int& folder_p, const int& folder_t) {
    //SetGenerationActivityParameters(true, path.filename().string(), folder_p, folder_t); // TODO
    //bool success = WriteSoundList(ProcessSoundSet(path), path.filename().string());
    //UpdateGenerationActivity(-1, true);
    //cout << (success ? " done" : " fail") << endl;
    //return success;
}

std::optional<SoundFileInfo> Preprocessor::GetSoundFileInfo (const std::filesystem::path& path) {
    {
        const std::string str_path = path.string();

        // Check path length

        if (str_path.length() > SOUNDPATH_MAXLEN)
        {
            //error_log << "[too long path] " << str_path << endl;
            return std::nullopt;
        }

        // Check if path is all lowercase.

        else if (any_of(str_path.begin(), str_path.end(), is_upper()))
        {
            //error_log << "[non-lowercase path] " << str_path << endl;
            return std::nullopt;
        }
    }

    if (path.has_extension())
    {
        std::string ext = path.extension().string();
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
                //error_log << "[invalid sample rate] " << path << ": " << properties->getSamplerate() << endl;
            } else {
                //error_log << "[invalid file] " << path << endl;
            }
        }
    }
    return std::nullopt;
}

SoundFileInfoList Preprocessor::ProcessSoundGroup (const std::filesystem::path& path) {
    SoundFileInfoList list;

    PathList paths;
    copy(std::filesystem::directory_iterator(path), std::filesystem::directory_iterator(), back_inserter(paths));
    sort(paths.begin(), paths.end(), cmp_ifspath); // To make sure it's sorted.

    for(PathList::const_iterator it (paths.begin()); it != paths.end(); ++it) {
        std::filesystem::path sub_path = std::filesystem::absolute((*it));

        if (std::filesystem::is_regular_file(sub_path)) {
            if (std::optional<SoundFileInfo> soundfileinfo = GetSoundFileInfo(*it)) {
                list.push_back(*soundfileinfo);
            }
        }
    }
    return list;
}

#include "preprocessor.hpp"
#include <iostream>

// libavformat
extern "C" {
#include <libavformat/avformat.h>
}

void Preprocessor::InitLibAV() {
    if (!avformat_init) {
        std::cout << "Initializing libavformat..." << std::endl;
        av_log_set_level(AV_LOG_ERROR);
        avformat_init = true;
    }
}

Preprocessor::Preprocessor (std::filesystem::path) {
    InitLibAV();
}

AliasMap Preprocessor::ParseAliasMap (const std::istream& input)
{
    AliasMap aliasmap;
    if (input.fail()) return aliasmap;

    std::string ln, source, alias, options;
    bool replace;
    int items_size;
    while (std::getline(input, ln))
    {
        boost::algorithm::trim(ln);

        if (!boost::algorithm::starts_with(ln, "#")) {
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
                        replace = true;
                }

                aliasmap.emplace_back(make_tuple(source, alias, replace));
            }
        }
    }

    return aliasmap;
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
                aliasmap = ParseAliasMap (std::ifstream (sub_path));
            }
            else
            {
                if (boost::optional<SoundFileInfo> soundinfo = GetSoundFileInfo (*it))
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
    bool success = WriteSoundList(ProcessSoundSet(path), path.filename().string());
    //UpdateGenerationActivity(-1, true);
    //cout << (success ? " done" : " fail") << endl;
    return success;
}

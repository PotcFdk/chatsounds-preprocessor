#include <fstream>
#include <numeric>

#include "types.hpp"
#include "file_util.hpp"
#include "util.hpp"

bool isMapPath (const std::filesystem::path& path) {
    return path.filename() == "map.txt";
}

std::optional<SoundProperties> getSoundProperties (const std::filesystem::path& path) {
    CPP_AVFormatContext ps;
    AVFormatContext *_ps = ps.get();
    avformat_open_input (ps.get_ptr(), path.string().c_str(), NULL, NULL);
    if (!ps) return std::nullopt;

    avformat_find_stream_info (_ps, NULL);
    int64_t duration = _ps->duration;
    if (duration <= 0) return std::nullopt;
    if (_ps->nb_streams != 1) return std::nullopt;

    return SoundProperties (
        Duration (static_cast<double>(duration)/AV_TIME_BASE),
        Samplerate (_ps->streams[0]->codecpar->sample_rate)
    );
}

std::optional<SoundFileInfo> proc_sound_file (const std::filesystem::path& path) {
    std::optional<SoundProperties> soundProperties = getSoundProperties (path);
    if (!soundProperties.has_value()) {
        return std::nullopt;
    } else {
        return SoundFileInfo (path, soundProperties.value());
    }
}

std::optional<SoundFileInfo> proc_sound_file_de (const std::filesystem::directory_entry& de) {
    return proc_sound_file (de.path());
}

AliasMap parseAliasMap (std::istream& input) {
    AliasMap aliasmap;
    if (input.fail()) return aliasmap;

    std::string ln, source, alias, options;
    bool replace;
    int items_size;
    while (std::getline(input, ln)) {
        boost::algorithm::trim(ln);

        if (!boost::algorithm::starts_with(ln, "#")) {
            std::vector<std::string> items;
            boost::algorithm::split(items, ln, match_char(';'));
            items_size = items.size();
            if ((items_size == 2 || items_size == 3) // source and alias exist
                && items.at(0).length() > 0 && items.at(1).length() > 0) // source and alias have content
            {
                source = boost::algorithm::trim_copy(items.at(0));
                alias  = boost::algorithm::trim_copy(items.at(1));

                boost::algorithm::to_lower(source);
                boost::algorithm::to_lower(alias);

                replace = false; // default behavior: don't replace, just alias

                if (items_size == 3) {
                    options = boost::algorithm::trim_copy(items.at(2));
                    boost::algorithm::to_lower(options);
                    if (boost::algorithm::contains(options, "replace"))
                        replace = true;
                }

                aliasmap.emplace_back (AliasMapEntry (SoundName (source), SoundName (alias), replace));
            }
        }
    }

    return aliasmap;
}

std::optional<AliasMap> proc_alias_file (const std::filesystem::path& path) {
    std::ifstream file (path);
    if (!file.is_open()) return std::nullopt;
    return parseAliasMap (file);
}

SoundFileInfoList proc_sound_group (const std::filesystem::directory_entry& de) {
    std::list<std::optional<SoundFileInfo>> list_optional_sfi;
    SoundFileInfoList sfil;

    DirectoryEntries entries = scandir (directory_entry_to_path (de));

    std::transform (entries.begin(), entries.end(), std::back_inserter(list_optional_sfi), proc_sound_file_de);
    list_optional_sfi.erase (std::remove_if (list_optional_sfi.begin(), list_optional_sfi.end(),
            [](auto e) { return !e.has_value(); }),
        list_optional_sfi.end());
    std::transform (list_optional_sfi.begin(), list_optional_sfi.end(), std::back_inserter(sfil), [](auto e) {
        // the sound group directory name defines the sound name, replace/fix it up
        e.value().setName (SoundName (e.value().getPath().parent_path().filename()));
        return e.value();
    });
    return sfil;
}

SoundName get_sound_name_from_SFIL (const SoundFileInfoList& sfil) {
    return sfil.front().getName(); // TODO: checks
}

SoundInfoMap proc_merge_SFIL_into_SIM (SoundInfoMap sim, SoundFileInfoList sfil) {
    // check if map contains key
    SoundName name (get_sound_name_from_SFIL (sfil));
    if (sim.find (name) != sim.end()) { // yes
        sim[name].splice(sim[name].begin(), sfil);
    } else { // no
        sim [name] = sfil;
    }
    return sim;
}

SoundInfoMap proc_apply_AME_to_SIM (SoundInfoMap sim, const AliasMapEntry& ame) {
    SoundName source (ame.getSource()), destination (ame.getDestination());
    if (sim.find (source) != sim.end()) { // found source in map
        if (sim.find (destination) != sim.end()) { // found destination in map -> merge
            std::copy(sim[source].begin(), sim[source].end(), std::back_insert_iterator<SoundFileInfoList>(sim[destination]));
            if (ame.getReplace())
                sim.erase (source);
        } else { // no destination -> simple move
            if (ame.getReplace()) {
                auto nodeHandler = sim.extract(source);
                nodeHandler.key() = destination;
                sim.insert(std::move(nodeHandler));
            } else {
                sim [destination] = sim [source];
            }
        }
    }
    return sim;
}

SoundInfoMap proc_apply_AM_to_SIM (SoundInfoMap sim, const AliasMap& am) {
    sim = std::accumulate (am.begin(), am.end(), sim, proc_apply_AME_to_SIM);
    return sim;
}

SoundInfoMap gen_SoundInfoMap (const DirectoryEntries& paths) {
    SoundInfoMap map;
    auto [files, directories] = split_files_directories (paths);
    files.erase (std::remove_if (files.begin(), files.end(), isMapPath), files.end());

    std::list<DirectoryEntries> list_de_files, list_de_directories;
    std::list<PathList> list_pl_directories;

    std::list<SoundFileInfoList> sfil_files, sfil_directories;

    { // handle files
        std::list<std::optional<SoundFileInfo>> list_optional_sfi_files;
        std::transform (files.begin(), files.end(), std::back_inserter(list_optional_sfi_files), proc_sound_file_de);
        list_optional_sfi_files.erase (std::remove_if (list_optional_sfi_files.begin(), list_optional_sfi_files.end(),
                [](auto e) { return !e.has_value(); }),
            list_optional_sfi_files.end());
        std::transform (list_optional_sfi_files.begin(), list_optional_sfi_files.end(), std::back_inserter(sfil_files), [](auto e) {
            return SoundFileInfoList { e.value() }; // wrap into a single-entry SFIL
        });
    }

    // handle directories / sound groups
    std::transform (directories.begin(), directories.end(), std::back_inserter(sfil_directories),
        proc_sound_group);

    // merge files and sound groups into the map
    map = std::accumulate (sfil_files.begin(), sfil_files.end(), map, proc_merge_SFIL_into_SIM);
    map = std::accumulate (sfil_directories.begin(), sfil_directories.end(), map, proc_merge_SFIL_into_SIM);

    return map;
}

SoundInfoMap gen_SoundInfoMap (const std::filesystem::directory_entry& de) {
    return gen_SoundInfoMap (scandir (de));
}

Repository gen_Repository (const std::filesystem::path& p) {
    Repository repository (scandir (p));
    std::sort (repository.get().begin(), repository.get().end(), cmp_ifspath);
    return repository;
}

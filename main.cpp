#include <iostream>
#include <filesystem.hpp>

#include <boost/program_options.hpp>

#include "src/constants.hpp"
#include "src/info.hpp"
#include "src/file_util.hpp"
#include "src/util.hpp"
#include "src/preprocessor.hpp"


int Launch_DiffUpdate(const bool &open_ext)
{
    Repository repository = gen_Repository (SOUNDPATH);

/*
    for (auto& e : repository) {
        for (auto& _e : e) {
            std::cout << _e << std::endl;
        }
    }
*/

    gen_SoundInfoMap (*(repository.begin()));

    return -1;
}

int Launch_FullUpdate(const bool &open_ext)
{
    EmptyDirectory (LISTPATH);
    return Launch_DiffUpdate (open_ext);
}

std::pair<std::string, std::string> win_help_cmd_param (const std::string& s)
{
    if (s == "/?") {
        return std::make_pair (std::string("help"), std::string());
    } else {
        return std::make_pair (std::string(), std::string());
    }
}

int main(int argc, char* argv[])
{
    boost::program_options::options_description commands("Commands");
    commands.add_options()
        ("help,h", "Usage help")
        ("version,v", "Show the program version")
        ("full,f", "Full, uncached list generation")
        ("diff,d", "Normal, cached list generation (default)")
        ("lite,l", "Same as --diff")
    ;

    boost::program_options::options_description parameters("Parameters");
    parameters.add_options()
        ("non-interactive", "non-interactive mode (don't expect any user input)")
    ;

    boost::program_options::options_description options("Allowed options");
    options.add(commands).add(parameters);

    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
            .options(options).extra_parser(win_help_cmd_param).run(), vm);
        boost::program_options::notify(vm);
    } catch (boost::program_options::unknown_option e) {
        print_topinfo();
        std::cout << e.what() << std::endl
                  << "For usage help, see -h or --help." << std::endl;
        return -4;
    }

    if (vm.count("version"))
    {
        print_versioninfo();
        return 0;
    }

    bool is_interactive = !vm.count("non-interactive");

    const bool open_ext = print_topinfo();

    if (vm.count("full"))
        return Launch_FullUpdate(open_ext);
    else if (vm.count("diff") || vm.count("lite"))
        return Launch_DiffUpdate(open_ext);
    else if (vm.count("help"))
    {
        std::cout << "Usage: " << std::endl
                  << " -f | --full     -  Full, uncached list generation" << std::endl
                  << " -d | --diff     -  Normal, cached list generation (default)" << std::endl
                  << " -l | --lite     -  Same as --diff" << std::endl
                  << " -h | --help     -  Usage help (this text right here)" << std::endl
                  << " -v | --version  -  Show the program version" << std::endl;
    }
    else
        return Launch_DiffUpdate(open_ext);

    return 0;
}
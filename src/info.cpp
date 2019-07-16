#include <version.hpp>
#include <iostream>

using std::cout, std::endl;

static const char
    *EXT_LINE = "                                                                      | |",
    *RXT_LINE = "                                                                      | |\r",
    *END_LINE = "                                                                      |/",
    *RND_LINE = "                                                                      |/\r",
    *NULL_CHR = "\0";

unsigned short month, day, year;

const char *months[] = {
    "Jan", "Feb", "Mar", "Apr", "May",
    "Jun", "Jul", "Aug","Sep", "Oct", "Nov", "Dec"
};

void readdate ()
{
    char temp [] = __DATE__;
    unsigned char i;

    year = atoi(temp + 7);
    *(temp + 6) = 0;
    day = atoi(temp + 4);
    *(temp + 3) = 0;
    for (i = 0; i < 12; i++)
    {
        if (!strcmp(temp, months[i]))
        {
            month = i + 1;
            return;
        }
    }
}

bool print_topinfo()
{
    readdate();

    const int tag_line_length = string("chatsounds-preprocessor v").length() + 2
        + intDigits(Version::MAJOR)
        + intDigits(Version::MINOR)
        + intDigits(Version::PATCH)
#if HAS_VERSION_DISTRIBUTION
        + strlen(Version::DISTRIBUTION) + 1
#endif
        + string(" by PotcFdk").length() - 1; // -1 for partly overwriting, looks cool

    cout << "        ______            ,                            _   __" << endl
         << "       / ___  |          /|                           | | /  \\\\" << endl
         << "      / /   | |__   __ _| |_ ___  ___  _   _ _ __   __| |/ /\\ \\\\" << endl
         << "     / /    | '_ \\ / _` | __/ __|/ _ \\| | | | `_ \\ / _` |\\ \\\\\\ \\\\" << endl
         << "    ( (     | | | | (_| | |_\\__ \\ (_) | |_| | | | | (_| | \\ \\\\\\//" << endl
         << "     \\ \\    |_| |_|\\__,_|\\______/\\___/ \\__,_|_| |_|\\__,/ \\ \\ \\\\" << endl
         << "      \\ \\____________                                   \\ \\/ //" << endl
         << "       \\____________ \\'\"`-._,-'\"`-._,-'\"`-._,-'\"`-._,-'\"`\\__//" << endl
         << "         ____  | |__) ) __ ___ _ __  _ __ ___   ___ ___  ___ ___  ___  _ __" << endl
         << "        (____) |  ___/ '__/ _ \\ '_ \\| '__/ _ \\ / __/ _ \\/ __/ __|/ _ \\| '__|" << endl
         << "               | |   | | |  __/ |_) | | | (_) | (_(  __/\\__ \\__ \\ (_) | |" << endl
         << "         _____/ /    | |  \\___| .__/|_|  \\___/ \\___\\________/___/\\___/| |" << endl
         << "        (______/     | |      | |                            ^potcfdk | |" << endl
         << "                      \\|      |_|                                     | |" << endl
         << (tag_line_length < 70 ? EXT_LINE : END_LINE)
         << endl;

    if (tag_line_length < 70)
        cout << RXT_LINE;

    cout << "chatsounds-preprocessor v"
         << Version::MAJOR << "."
         << Version::MINOR << "."
         << Version::PATCH
#if HAS_VERSION_DISTRIBUTION
         << Version::DISTRIBUTION
#endif
         << " by PotcFdk" << endl;

    if (tag_line_length < 70)
        cout << EXT_LINE << endl << RXT_LINE;
    else
        cout << endl;

    cout << "Please report any bugs / issues to:" << endl;

    if (tag_line_length < 70)
        cout << RXT_LINE;

    cout << BUGTRACKER_LINK << endl;

    if (tag_line_length < 70)
    {
        cout << EXT_LINE << endl << RXT_LINE;
        return true;
    }
    else
    {
        cout << endl;
        return tag_line_length < 70;
    }
}

void print_versioninfo()
{
    getdate();

    cout << "chatsounds-preprocessor"
         << endl << "Author     : PotcFdk"
         << endl << "Version    : "
         << Version::MAJOR << "."
         << Version::MINOR << "."
         << Version::PATCH
#if HAS_VERSION_DISTRIBUTION
         << Version::DISTRIBUTION
#endif
         << endl << "Build date : "
         << year << "-";
    if (month < 10) cout << 0;
    cout << month << "-";
    if (day < 10) cout << 0;
    cout << day << endl
#if defined(__clang__)
         << "Compiler   : Clang/LLVM, version "
#if defined(__clang_major__) && defined(__clang_minor__) && defined(__clang_patchlevel__)
         << __clang_major__ << '.' << __clang_minor__ << '.' << __clang_patchlevel__
#else
         << __VERSION__
#endif
         << endl;
#elif defined(__GNUG__)
         << "Compiler   : GNU G++, version "
#if defined(__GNUG__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
         << __GNUG__ << '.' << __GNUC_MINOR__ << '.' << __GNUC_PATCHLEVEL__
#else
         << __VERSION__
#endif
         << endl;
#elif defined(_MSC_VER)
         << "Compiler   : Microsoft Visual Studio, version "  << _MSC_VER << endl;
#elif defined(__VERSION__)
         << "Compiler   : Unknown compiler, version " << __VERSION__ << endl;
#else
         << "Compiler   : Unknown compiler" << endl;
#endif

    // More verbose information

    cout << "Boost Info : " << endl
         << " * Boost version " << (BOOST_VERSION / 100000)
         << '.' << ((BOOST_VERSION / 100) % 1000)
         << '.' << (BOOST_VERSION % 100) << endl
         << " * " << BOOST_COMPILER << " on " << BOOST_PLATFORM << endl
         << " * " << BOOST_STDLIB << endl;

    cout << "libav Info : " << endl
         << " * libavformat " << AV_STRINGIFY(LIBAVFORMAT_VERSION) << " (ct:" << LIBAVFORMAT_VERSION_INT << " rt:" << avformat_version() << ")" << endl
         << " * libavcodec  " << AV_STRINGIFY(LIBAVCODEC_VERSION)  << " (ct:" << LIBAVCODEC_VERSION_INT  << " rt:" << avcodec_version()  << ")" << endl
         << " * libavutil   " << AV_STRINGIFY(LIBAVUTIL_VERSION)   << " (ct:" << LIBAVUTIL_VERSION_INT   << " rt:" << avutil_version()   << ")" << endl;
}
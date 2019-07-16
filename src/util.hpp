#ifndef CP_UTIL_HPP
#define CP_UTIL_HPP

#include <filesystem.hpp>
#include "types.hpp"

std::filesystem::path strip_root (const std::filesystem::path&);
int getNumberOfDirectories (std::filesystem::path);
bool cmp_ifspath (const std::filesystem::path&, const std::filesystem::path&);
bool cmp_sfi (const SoundFileInfo&, const SoundFileInfo&);
int intDigits (int);

struct is_upper {
    bool operator() (int value) {
        return ::isupper ((unsigned char) value);
    }
};

struct match_char {
    char c;
    match_char(char c) : c(c) {}
    bool operator()(char x) const {
        return x == c;
    }
};

#endif
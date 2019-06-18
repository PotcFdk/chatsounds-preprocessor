#include <iostream>
#include <fstream>
#include <filesystem.hpp>

class ErrorLogger
{
public:
    explicit ErrorLogger (std::filesystem::path path) {
        invalid_file_log.open (path);
    }

    void reset () { in_line = true; }

    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    typedef CoutType& (*StandardEndLine)(CoutType&);

    ErrorLogger& operator << (StandardEndLine manip)
    {
        invalid_file_log << std::endl;
        std::cout << std::endl;
        in_line = false;
        return *this;
    }

    template <typename T> ErrorLogger& operator << (const T& value)
    {
        invalid_file_log << value;
        if (in_line)
        {
            std::cout << std::endl;
            in_line = false;
        }
        std::cout << value;
        return *this;
    }
private:
    bool in_line = true;
    std::ofstream invalid_file_log;
};
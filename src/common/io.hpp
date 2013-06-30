#ifndef TMWA_COMMON_IO_HPP
#define TMWA_COMMON_IO_HPP

#include <istream>
#include <ostream>

#include "strings.hpp"

namespace io
{
    inline
    std::istream& getline(std::istream& in, FString& line)
    {
        std::string s;
        if (std::getline(in, s))
        {
            std::string::const_iterator begin = s.cbegin(), end = s.cend();
            if (begin != end && end[-1] == '\r')
                --end;
            line = FString(begin, end);
        }
        return in;
    }
} // namespace io

#endif //TMWA_COMMON_IO_HPP

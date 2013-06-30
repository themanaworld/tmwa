#ifndef INTERN_POOL_HPP
#define INTERN_POOL_HPP

#include <cassert>

#include <map>
#include <vector>

#include "strings.hpp"

class InternPool
{
    std::map<FString, size_t> known;
    std::vector<FString> names;
public:
    size_t intern(XString name_)
    {
        FString name = name_;
        // hm, I could change this to do aliases
        auto pair = known.insert({name, known.size()});
        if (pair.second)
            names.push_back(name);
        assert (known.size() == names.size());
        return pair.first->second;
    }

    ZString outtern(size_t sz) const
    {
        return names[sz];
    }

    size_t size() const
    {
        return known.size();
    }
};

#endif //INTERN_POOL_HPP

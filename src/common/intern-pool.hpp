#ifndef INTERN_POOL_HPP
#define INTERN_POOL_HPP

#include <cassert>

#include <map>
#include <string>
#include <vector>

class InternPool
{
    std::map<std::string, size_t> known;
    std::vector<std::string> names;
public:
    size_t intern(const std::string& name)
    {
        auto pair = known.insert({name, known.size()});
        if (pair.second)
            names.push_back(name);
        assert (known.size() == names.size());
        return pair.first->second;
    }

    const std::string& outtern(size_t sz) const
    {
        return names[sz];
    }

    size_t size() const
    {
        return known.size();
    }
};

#endif //INTERN_POOL_HPP

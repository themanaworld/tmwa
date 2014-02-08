#ifndef INTERN_POOL_HPP
#define INTERN_POOL_HPP

# include <cassert>

# include <map>
# include <vector>

# include "../strings/rstring.hpp"
# include "../strings/zstring.hpp"
# include "../strings/xstring.hpp"

class InternPool
{
    std::map<RString, size_t> known;
    std::vector<RString> names;
public:
    size_t intern(XString name_)
    {
        // TODO just look up the XString, the memory should not move by now
        RString name = name_;
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

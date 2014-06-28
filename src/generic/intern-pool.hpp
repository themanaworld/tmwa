#pragma once
//    intern-pool.hpp - Cached integer/string lookups.
//
//    Copyright © 2013 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "fwd.hpp"

#include <cassert>
#include <cstddef>

#include <map>
#include <vector>

#include "../strings/rstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"


namespace tmwa
{
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
        assert (sz < names.size());
        return names[sz];
    }

    size_t size() const
    {
        return known.size();
    }
};
} // namespace tmwa

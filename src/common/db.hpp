#ifndef DB_HPP
#define DB_HPP
//    db.hpp - convenience wrappers over std::map<K, V>
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

# include "sanity.hpp"

# include <map>

template<class K, class V>
class Map
{
    typedef std::map<K, V> Impl;

    Impl impl;
public:
    Map() = default;
    Map(std::initializer_list<std::pair<const K, V>> il)
    : impl(il)
    {}
    typedef typename Impl::iterator iterator;
    typedef typename Impl::const_iterator const_iterator;

    iterator begin() { return impl.begin(); }
    iterator end() { return impl.end(); }
    const_iterator begin() const { return impl.begin(); }
    const_iterator end() const { return impl.end(); }

    V *search(const K& k)
    {
        iterator it = impl.find(k);
        if (it == impl.end())
            return nullptr;
        return &it->second;
    }
    const V *search(const K& k) const
    {
        const_iterator it = impl.find(k);
        if (it == impl.end())
            return nullptr;
        return &it->second;
    }
    void insert(K k, V v)
    {
        // As far as I can tell, this is the simplest way to
        // implement move-only insert-with-replacement.
        iterator it = impl.lower_bound(k);
        // invariant: if it is valid, it->first >= k
        if (it != impl.end() && it->first == k)
            it->second = std::move(v);
        else
            it = impl.insert(std::pair<K, V>(std::move(k), std::move(v))).first;
        return (void)&it->second;

    }
    V *init(K k)
    {
        return &impl[k];
    }
    void erase(const K& k)
    {
        impl.erase(k);
    }
    void clear()
    {
        impl.clear();
    }
    bool empty()
    {
        return impl.empty();
    }
};

template<class K, class V>
class DMap
{
    typedef Map<K, V> Impl;

    Impl impl;
public:
    typedef typename Impl::iterator iterator;
    typedef typename Impl::const_iterator const_iterator;

    iterator begin() { return impl.begin(); }
    iterator end() { return impl.end(); }
    const_iterator begin() const { return impl.begin(); }
    const_iterator end() const { return impl.end(); }

    V get(K k)
    {
        V *vp = impl.search(k);
        return vp ? *vp : V();
    }
    void put(K k, V v)
    {
        if (v == V())
            impl.erase(k);
        else
            impl.insert(k, v);
    }
    void clear()
    {
        impl.clear();
    }
};

#endif // DB_HPP

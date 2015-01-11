#pragma once
//    generic/fwd.hpp - list of type names for generic lib
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "../sanity.hpp"

#include "../strings/fwd.hpp" // rank 1
#include "../compat/fwd.hpp" // rank 2
// generic/fwd.hpp is rank 3


namespace tmwa
{
// meh, add more when I feel like it
template<class T>
class dumb_ptr;

template<class K, class V>
class Map;
template<class K, class V>
class DMap;
template<class K, class V>
class UPMap;

class InternPool;

// arrays are complicated
template<class I, I be, I en>
struct ExclusiveIndexing;
template<size_t n>
using SimpleIndexing = ExclusiveIndexing<size_t, 0, n>;
template<class I, I lo, I hi>
struct InclusiveIndexing;
template<class E, E n=E::COUNT>
struct EnumIndexing;
template<class I, size_t limit>
struct InventoryIndexing;
template<class T, class I>
struct GenericArray;
template<class T, size_t n>
using Array = GenericArray<T, SimpleIndexing<n>>;

template<class T, class E, E max>
using earray = GenericArray<T, EnumIndexing<E, max>>;
} // namespace tmwa

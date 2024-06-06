#pragma once
//    login/fwd.hpp - list of type names for login server
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

#include "sanity.hpp"

#include "../ints/fwd.hpp" // rank 1
#include "../strings/fwd.hpp" // rank 1
#include "../compat/fwd.hpp"
#include "../generic/fwd.hpp" // rank 3
#include "../io/fwd.hpp" // rank 4
#include "../net/fwd.hpp" // rank 5
#include "../mmo/fwd.hpp" // rank 6
#include "proto2/fwd.hpp" // rank 8
#include "../high/fwd.hpp" // rank 9
#include "../wire/fwd.hpp" // rank 9
// login/fwd.hpp is rank ∞ because it is an executable


namespace tmwa
{
namespace login
{
    struct LoginConf;
    struct LoginLanConf;
    struct mmo_char_server;
    struct AuthData;
    // meh, add more when I feel like it
} // namespace login
} // namespace tmwa

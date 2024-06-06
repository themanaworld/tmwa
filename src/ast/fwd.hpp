#pragma once
//    ast/fwd.hpp - list of type names for new independent tmwa ast
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

#include "../compat/fwd.hpp" // rank 2
#include "../io/fwd.hpp" // rank 4
#include "../net/fwd.hpp" // rank 5
#include "../sexpr/fwd.hpp" // rank 5
#include "../mmo/fwd.hpp" // rank 6
#include "../high/fwd.hpp" // rank 9
// ast/fwd.hpp is rank 10

namespace tmwa
{
namespace ast
{
namespace npc
{
class Warp;
} // namespace npc
namespace script
{
class ScriptBody;
} // namespace script
} // namespace ast
// meh, add more when I feel like it
} // namespace tmwa

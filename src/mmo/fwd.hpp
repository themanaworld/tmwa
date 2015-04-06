#pragma once
//    mmo/fwd.hpp - list of type names for mmo lib
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

#include "../ints/fwd.hpp" // rank 1
#include "../strings/fwd.hpp" // rank 1
#include "../compat/fwd.hpp" // rank 2
#include "../generic/fwd.hpp" // rank 3
#include "../io/fwd.hpp" // rank 4
#include "../net/fwd.hpp" // rank 5
// mmo/fwd.hpp is rank 6


namespace tmwa
{
// meh, add more when I feel like it
class MapName;
class CharName;

class HumanTimeDiff;

class Species;
class AccountId;
class CharId;
class PartyId;
class ItemNameId;
class BlockId;
class GmLevel;

class AccountName;
class AccountPass;
class AccountCrypt;
class AccountEmail;
class ServerName;
class PartyName;
class QuestId;
class VarName;
class MapName;
class CharName;

struct MobName;
struct NpcName;
struct ScriptLabel;
struct ItemName;
struct NpcEvent;
} // namespace tmwa

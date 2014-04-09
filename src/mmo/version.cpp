#include "version.hpp"
//    version.cpp - Prevent mass rebuild when conf/version.hpp changes.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "../conf/version.hpp"

#include "../strings/xstring.hpp"

#include "extract.hpp"

#include "../poison.hpp"

Version CURRENT_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, 0,
    VENDOR_POINT,
};
Version CURRENT_LOGIN_SERVER_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, TMWA_SERVER_LOGIN,
    VENDOR_POINT,
};
Version CURRENT_CHAR_SERVER_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, TMWA_SERVER_CHAR | TMWA_SERVER_INTER,
    VENDOR_POINT,
};
Version CURRENT_MAP_SERVER_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, TMWA_SERVER_MAP,
    VENDOR_POINT,
};

const char CURRENT_VERSION_STRING[] = VERSION_STRING;

bool extract(XString str, Version *vers)
{
    *vers = {};
    // TODO should I try to extract dev and vend also?
    // It would've been useful during the magic migration.
    return extract(str, record<'.'>(&vers->major, &vers->minor, &vers->patch));
}

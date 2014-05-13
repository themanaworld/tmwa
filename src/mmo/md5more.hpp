#ifndef TMWA_MMO_MD5MORE_HPP
#define TMWA_MMO_MD5MORE_HPP
//    md5more.hpp - Non-basic MD5 functions.
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

# include "fwd.hpp"

# include "../generic/md5.hpp"

# include "../io/fwd.hpp"

# include "../net/fwd.hpp"

MD5_state MD5_from_FILE(io::ReadFile& in);

// whoever wrote this fails basic understanding of
AccountCrypt MD5_saltcrypt(AccountPass key, SaltString salt);

/// return some random characters
// Currently, returns a 5-char string
SaltString make_salt(void);

/// check plaintext password against saved saltcrypt
bool pass_ok(AccountPass password, AccountCrypt crypted);

/// This returns an IP4Address because it is configurable whether it gets called at all
IP4Address MD5_ip(IP4Address ip);

#endif // TMWA_MMO_MD5MORE_HPP

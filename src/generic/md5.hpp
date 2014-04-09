#ifndef TMWA_GENERIC_MD5CALC_HPP
#define TMWA_GENERIC_MD5CALC_HPP
//    md5.hpp - Fundamental MD5 operations.
//
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

# include "../sanity.hpp"

# include <netinet/in.h>

# include <cstdint>
# include <cstddef>
# include <cstdio>

# include <array>

# include "../strings/fwd.hpp"
# include "../strings/vstring.hpp"

/// The digest state - becomes the output
struct MD5_state
{
    // classically named {A,B,C,D}
    // but use an array so we can index
    uint32_t val[4];
};
struct MD5_block
{
    uint32_t data[16];
};

struct md5_binary : std::array<uint8_t, 0x10> {};
struct md5_string : VString<0x20> {};
struct SaltString : VString<5> {};

// Implementation
void MD5_init(MD5_state *state);
void MD5_do_block(MD5_state *state, MD5_block block);

// Output formatting
void MD5_to_bin(MD5_state state, md5_binary& out);
void MD5_to_str(MD5_state state, md5_string& out);

// Convenience
MD5_state MD5_from_string(XString msg);

#endif // TMWA_GENERIC_MD5CALC_HPP

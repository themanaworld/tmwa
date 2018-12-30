#pragma once
//    login.hpp - dummy header to make Make dependencies work.
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

#include "../strings/vstring.hpp"

#include "../compat/time_t.hpp"

#include "../generic/array.hpp"

#include "../net/ip.hpp"
#include "../net/timestamp-utils.hpp"

#include "../mmo/consts.hpp"
#include "../mmo/enums.hpp"
#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"

#include "../proto2/net-GlobalReg.hpp"

#include "../high/mmo.hpp"


namespace tmwa
{
namespace login
{
constexpr AccountId START_ACCOUNT_NUM = wrap<AccountId>(2000000);
constexpr AccountId END_ACCOUNT_NUM = wrap<AccountId>(100000000);

struct AuthData
{
    AccountId account_id;
    SEX sex;
    AccountName userid;
    AccountCrypt pass;
    timestamp_milliseconds_buffer lastlogin;
    int logincount;
    int state;                 // packet 0x006a value + 1 (0: compte OK)
    AccountEmail email;             // e-mail (by default: a@a.com)
    timestamp_seconds_buffer error_message;     // Message of error code #6 = Your are Prohibited to log in until %s (packet 0x006a)
    TimeT ban_until_time;      // # of seconds 1/1/1970 (timestamp): ban time limit of the account (0 = no ban)
    IP4Address last_ip;           // save of last IP of connection
    VString<254> memo;             // a memo field
    int account_reg2_num;
    Array<GlobalReg, ACCOUNT_REG2_NUM> account_reg2;
};

struct mmo_char_server
{
    ServerName name;
    IP4Address ip;
    uint16_t port;
    uint16_t users;
};

struct AuthFifo
{
    AccountId account_id;
    int login_id1, login_id2;
    IP4Address ip;
    SEX sex;
    int delflag;
    int consumed_by;
    ClientVersion client_version;
};
} // namespace login
} // namespace tmwa

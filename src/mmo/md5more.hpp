#ifndef TMWA_MMO_MD5MORE_HPP
#define TMWA_MMO_MD5MORE_HPP

# include "../generic/md5.hpp"

# include "../io/read.hpp"

# include "ip.hpp"
# include "mmo.hpp"

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

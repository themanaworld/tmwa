#ifndef TMWA_CONF_VERSION_HPP
#define TMWA_CONF_VERSION_HPP
//    conf/version.hpp - Import configuration variables related to version.
//
//    Copyright © 2013-2014 Ben Longbons <b.r.longbons@gmail.com>
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

// just mention "fwd.hpp" to make formatter happy

# include "conf-raw/str-VERSION_FULL.h"
# include "conf-raw/str-VERSION_HASH.h"

# include "conf-raw/int-VERSION_MAJOR.h"
# include "conf-raw/int-VERSION_MINOR.h"
# include "conf-raw/int-VERSION_PATCH.h"
# include "conf-raw/int-VERSION_DEVEL.h"

# include "conf-raw/str-VENDOR_NAME.h"
# include "conf-raw/int-VENDOR_POINT.h"
# include "conf-raw/str-VENDOR_SOURCE.h"

# include "conf-raw/str-VERSION_STRING.h"


namespace tmwa
{
} // namespace tmwa

#endif // TMWA_CONF_VERSION_HPP

#ifndef TMWA_MAP_TMW_HPP
#define TMWA_MAP_TMW_HPP

# include "../strings/fwd.hpp"

# include "../generic/const_array.hpp"

# include "../mmo/dumb_ptr.hpp"

# include "map.hpp"

int tmw_CheckChatSpam(dumb_ptr<map_session_data> sd, XString message);
void tmw_GmHackMsg(ZString line);

#endif // TMWA_MAP_TMW_HPP

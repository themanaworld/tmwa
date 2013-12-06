#ifndef TMW_HPP
#define TMW_HPP

# include "../strings/fwd.hpp"

# include "../common/const_array.hpp"
# include "../common/dumb_ptr.hpp"

# include "map.hpp"

int tmw_CheckChatSpam(dumb_ptr<map_session_data> sd, XString message);
void tmw_GmHackMsg(ZString line);

#endif // TMW_HPP

#ifndef TMWA_MAP_ATCOMMAND_HPP
#define TMWA_MAP_ATCOMMAND_HPP

# include "../strings/fwd.hpp"

# include "map.hpp"

bool is_atcommand(Session *s, dumb_ptr<map_session_data> sd,
        ZString message, int gmlvl);

bool atcommand_config_read(ZString cfgName);

void log_atcommand(dumb_ptr<map_session_data> sd, ZString cmd);

// only used by map.cpp
extern AString gm_log;

void atcommand_config_write(ZString cfgName);

#endif // TMWA_MAP_ATCOMMAND_HPP

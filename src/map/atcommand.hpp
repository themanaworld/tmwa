#ifndef ATCOMMAND_HPP
#define ATCOMMAND_HPP

#include "../common/const_array.hpp"

#include "map.hpp"

bool is_atcommand(const int fd, dumb_ptr<map_session_data> sd,
        ZString message, int gmlvl);

int atcommand_config_read(ZString cfgName);

void log_atcommand(dumb_ptr<map_session_data> sd, ZString cmd);

// only used by map.cpp
extern FString gm_logfile_name;

#endif // ATCOMMAND_HPP

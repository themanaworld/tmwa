#ifndef ATCOMMAND_HPP
#define ATCOMMAND_HPP

#include "../common/const_array.hpp"

#include "map.hpp"

bool is_atcommand(const int fd, dumb_ptr<map_session_data> sd,
        const char *message, int gmlvl);

int atcommand_config_read(const char *cfgName);

void log_atcommand(dumb_ptr<map_session_data> sd, const_string cmd);

// only used by map.cpp
extern std::string gm_logfile_name;

#endif // ATCOMMAND_HPP

#ifndef ATCOMMAND_HPP
#define ATCOMMAND_HPP

#include "../common/const_array.hpp"

bool is_atcommand(const int fd, struct map_session_data *sd,
        const char *message, int gmlvl);

int atcommand_config_read(const char *cfgName);

void log_atcommand(struct map_session_data *sd, const_string cmd);

// only used by map.cpp
extern char *gm_logfile_name;

#endif // ATCOMMAND_HPP

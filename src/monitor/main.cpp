//    monitor/main.cpp - Old daemon to restart servers when they crashed.
//
//    Copyright © ???? Bartosz Waszak <waszi@evil.org.pl>
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

#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/fd.hpp"
#include "../io/read.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/utils.hpp"

#include "../poison.hpp"

#define LOGIN_SERVER "./login-server"_s
#define MAP_SERVER "./map-server"_s
#define CHAR_SERVER "./char-server"_s
#define CONFIG "conf/eathena-monitor.conf"_s


// initialiized to $HOME/tmwserver
static
AString workdir;
//the rest are relative to workdir
static
AString login_server = LOGIN_SERVER;
static
AString map_server = MAP_SERVER;
static
AString char_server = CHAR_SERVER;

static
pid_t pid_login, pid_map, pid_char;

static
AString make_path(XString base, XString path)
{
    MString m;
    m += base;
    m += '/';
    m += path;
    return AString(m);
}

static
bool parse_option(XString name, ZString value)
{
    if (name == "login_server"_s)
        login_server = value;
    else if (name == "map_server"_s)
        map_server = value;
    else if (name == "char_server"_s)
        char_server = value;
    else if (name == "workdir"_s)
        workdir = value;
    else
    {
        FPRINTF(stderr, "WARNING: ingnoring invalid option '%s' : '%s'\n"_fmt,
                AString(name), value);
        return false;
    }
    return true;
}

static
bool read_config(ZString filename)
{
    bool rv = true;
    io::ReadFile in(filename);
    if (!in.is_open())
    {
        FPRINTF(stderr, "Monitor config file not found: %s\n"_fmt, filename);
        exit(1);
    }

    AString line;
    while (in.getline(line))
    {
        if (is_comment(line))
            continue;
        XString name;
        ZString value;
        if (!config_split(line, &name, &value))
        {
            PRINTF("Bad line: %s\n"_fmt, line);
            rv = false;
            continue;
        }

        if (!parse_option(name, value))
        {
            PRINTF("Bad key/value: %s\n"_fmt, line);
            rv = false;
            continue;
        }
    }
    return rv;
}

static
pid_t start_process(ZString exec)
{
    const char *args[2] = {exec.c_str(), NULL};
    pid_t pid = fork();
    if (pid == -1)
    {
        FPRINTF(stderr, "Failed to fork"_fmt);
        return 0;
    }
    if (pid == 0)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
        execv(exec.c_str(), const_cast<char **>(args));
#pragma GCC diagnostic pop
        perror("Failed to exec");
        kill(getppid(), SIGABRT);
        exit(1);
    }
    return pid;
}

// Kill all children with the same signal we got, then ourself.
static
void stop_process(int sig)
{
    if (pid_map)
        kill(pid_map, sig);
    if (pid_login)
        kill(pid_login, sig);
    if (pid_char)
        kill(pid_char, sig);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    signal(sig, SIG_DFL);
#pragma GCC diagnostic pop
    raise(sig);
}

int main(int argc, char *argv[])
{
    // These are all the signals we are likely to get
    // The shell handles stop/cont
    signal(SIGTERM, stop_process);
    signal(SIGINT, stop_process);
    signal(SIGQUIT, stop_process);
    signal(SIGABRT, stop_process);

    workdir = make_path(ZString(strings::really_construct_from_a_pointer, getenv("HOME"), nullptr), "tmwserver"_s);

    ZString config = CONFIG;
    if (argc > 1)
        config = ZString(strings::really_construct_from_a_pointer, argv[1], nullptr);
    read_config(config);

    if (chdir(workdir.c_str()) < 0)
    {
        perror("Failed to change directory");
        exit(1);
    }

    PRINTF("Starting:\n"_fmt);
    PRINTF("* workdir: %s\n"_fmt,  workdir);
    PRINTF("* login_server: %s\n"_fmt, login_server);
    PRINTF("* char_server: %s\n"_fmt, char_server);
    PRINTF("* map_server: %s\n"_fmt, map_server);
    {
        //make sure all possible file descriptors are free for use by the servers
        //if there are file descriptors higher than the max open from before the limit dropped, that's not our problem
        io::FD fd = io::FD::sysconf_SC_OPEN_MAX();
        while ((fd = fd.prev()) > io::FD::stderr())
        {
            if (fd.close() == 0)
                FPRINTF(stderr, "close fd %d\n"_fmt, fd.uncast_dammit());
        }
        fd = io::FD::open("/dev/null"_s, O_RDWR);
        if (fd == io::FD())
        {
            perror("open /dev/null");
            exit(1);
        }
        fd.dup2(io::FD::stdin());
        fd.dup2(io::FD::stdout());
        fd.close();
    }
    while (1)
    {
        // write stuff to stderr
        timestamp_seconds_buffer timestamp;
        stamp_time(timestamp);

        if (!pid_login)
        {
            pid_login = start_process(login_server);
            FPRINTF(stderr, "[%s] forked login server: %lu\n"_fmt,
                    timestamp, static_cast<unsigned long>(pid_login));
        }
        if (!pid_char)
        {
            pid_char = start_process(char_server);
            FPRINTF(stderr, "[%s] forked char server: %lu\n"_fmt,
                    timestamp, static_cast<unsigned long>(pid_char));
        }
        if (!pid_map)
        {
            pid_map = start_process(map_server);
            FPRINTF(stderr, "[%s] forked map server: %lu\n"_fmt,
                    timestamp, static_cast<unsigned long>(pid_map));
        }
        pid_t dead = wait(NULL);
        if (dead == -1)
        {
            perror("Failed to wait for child");
            exit(1);
        }
        if (pid_login == dead)
            pid_login = 0;
        if (pid_char == dead)
            pid_char = 0;
        if (pid_map == dead)
            pid_map = 0;
    }
}

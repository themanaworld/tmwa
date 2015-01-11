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
#include "../strings/literal.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/fd.hpp"
#include "../io/line.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/version.hpp"

#include "../net/timestamp-utils.hpp"

#include "globals.hpp"
#include "monitor_conf.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace monitor
{
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
pid_t start_process(ZString exec)
{
    const char *args[2] = {exec.c_str(), nullptr};
    pid_t pid = fork();
    if (pid == -1)
    {
        FPRINTF(stderr, "Failed to fork"_fmt);
        return 0;
    }
    if (pid == 0)
    {
        DIAG_PUSH();
        DIAG_I(cast_qual);
        execv(exec.c_str(), const_cast<char **>(args));
        DIAG_POP();
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
    if (pid_login)
        kill(pid_login, sig);
    if (pid_char)
        kill(pid_char, sig);
    if (pid_map)
        kill(pid_map, sig);
    DIAG_PUSH();
    DIAG_I(old_style_cast);
    DIAG_I(zero_as_null_pointer_constant);
    signal(sig, SIG_DFL);
    DIAG_POP();
    raise(sig);
}

static
bool monitor_config(io::Spanned<XString> key, io::Spanned<ZString> value)
{
    return parse_monitor_conf(monitor_conf, key, value);
}

static
bool monitor_confs(io::Spanned<XString> key, io::Spanned<ZString> value)
{
    if (key.data == "monitor_conf"_s)
    {
        return load_config_file(value.data, monitor_config);
    }
    key.span.error("Unknown meta-key for monitor nonserver"_s);
    return false;
}
} // namespace monitor
} // namespace tmwa

int main(int argc, char *argv[])
{
    using namespace tmwa;
    using namespace tmwa::monitor;
    // These are all the signals we are likely to get
    // The shell handles stop/cont
    signal(SIGTERM, stop_process);
    signal(SIGINT, stop_process);
    signal(SIGQUIT, stop_process);
    signal(SIGABRT, stop_process);

    monitor_conf.workdir = make_path(ZString(strings::really_construct_from_a_pointer, getenv("HOME"), nullptr), "tmwserver"_s);

    ZString argv0 = ZString(strings::really_construct_from_a_pointer, argv[0], nullptr);
    bool loaded_config_yet = false;
    bool runflag = true;

    for (int ai = 1; ai < argc; ++ai)
    {
        ZString argvi = ZString(strings::really_construct_from_a_pointer, argv[ai], nullptr);
        if (argvi.startswith('-'))
        {
            if (argvi == "--help"_s)
            {
                PRINTF("Usage: %s [--help] [--version] [files...]\n"_fmt,
                        argv0);
                exit(0);
            }
            else if (argvi == "--version"_s)
            {
                PRINTF("%s\n"_fmt, CURRENT_VERSION_STRING);
                exit(0);
            }
            else
            {
                FPRINTF(stderr, "Unknown argument: %s\n"_fmt, argvi);
                runflag = false;
            }
        }
        else
        {
            loaded_config_yet = true;
            runflag &= load_config_file(argvi, monitor_confs);
        }
    }

    if (!loaded_config_yet)
        runflag &= load_config_file("conf/tmwa-monitor.conf"_s, monitor_confs);

    if (!runflag)
        exit(1);

    if (chdir(monitor_conf.workdir.c_str()) < 0)
    {
        perror("Failed to change directory");
        exit(1);
    }

    PRINTF("Starting:\n"_fmt);
    PRINTF("* workdir: %s\n"_fmt, monitor_conf.workdir);
    PRINTF("* login_server: %s\n"_fmt, monitor_conf.login_server);
    PRINTF("* char_server: %s\n"_fmt, monitor_conf.char_server);
    PRINTF("* map_server: %s\n"_fmt, monitor_conf.map_server);
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
            pid_login = start_process(monitor_conf.login_server);
            FPRINTF(stderr, "[%s] forked login server: %lu\n"_fmt,
                    timestamp, static_cast<unsigned long>(pid_login));
        }
        if (!pid_char)
        {
            pid_char = start_process(monitor_conf.char_server);
            FPRINTF(stderr, "[%s] forked char server: %lu\n"_fmt,
                    timestamp, static_cast<unsigned long>(pid_char));
        }
        if (!pid_map)
        {
            pid_map = start_process(monitor_conf.map_server);
            FPRINTF(stderr, "[%s] forked map server: %lu\n"_fmt,
                    timestamp, static_cast<unsigned long>(pid_map));
        }
        pid_t dead = wait(nullptr);
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

/**
 * Name:    eAthena processes monitor
 * Original Author:  Bartosz Waszak <waszi@evil.org.pl>
 * Rewrite Author: Ben Longbons <b.r.longbons@gmail.com>
 * License: GPL
 * Compilation:
 * gcc -o eathena-monitor eathena-monitor.c
 */

#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include <csignal>

#include <fstream>

#include "../strings/mstring.hpp"
#include "../strings/fstring.hpp"
#include "../strings/tstring.hpp"
#include "../strings/sstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../common/cxxstdio.hpp"
#include "../common/io.hpp"
#include "../common/utils.hpp"

#include "../poison.hpp"

#define LOGIN_SERVER "./login-server"
#define MAP_SERVER "./map-server"
#define CHAR_SERVER "./char-server"
#define CONFIG "conf/eathena-monitor.conf"


// initialiized to $HOME/tmwserver
static
FString workdir;
//the rest are relative to workdir
static
FString login_server = LOGIN_SERVER;
static
FString map_server = MAP_SERVER;
static
FString char_server = CHAR_SERVER;

static
pid_t pid_login, pid_map, pid_char;

static
FString make_path(XString base, XString path)
{
    MString m;
    m += base;
    m += '/';
    m += path;
    return FString(m);
}

static
void parse_option(XString name, ZString value)
{
    if (name == "login_server")
        login_server = value;
    else if (name == "map_server")
        map_server = value;
    else if (name == "char_server")
        char_server = value;
    else if (name == "workdir")
        workdir = value;
    else
    {
        FString name_ = name;
        FPRINTF(stderr, "WARNING: ingnoring invalid option '%s' : '%s'\n",
                name_, value);
    }
}

static
void read_config(ZString filename)
{
    std::ifstream in(filename.c_str());
    if (!in.is_open())
    {
        FPRINTF(stderr, "Monitor config file not found: %s\n", filename);
        exit(1);
    }

    FString line;
    while (io::getline(in, line))
    {
        SString name;
        TString value;
        if (!split_key_value(line, &name, &value))
            continue;

        parse_option(name, value);
    }
}

static
pid_t start_process(ZString exec)
{
    const char *args[2] = {exec.c_str(), NULL};
    pid_t pid = fork();
    if (pid == -1)
    {
        FPRINTF(stderr, "Failed to fork");
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

    workdir = make_path(ZString(strings::really_construct_from_a_pointer, getenv("HOME"), nullptr), "tmwserver");

    ZString config = CONFIG;
    if (argc > 1)
        config = ZString(strings::really_construct_from_a_pointer, argv[1], nullptr);
    read_config(config);

    if (chdir(workdir.c_str()) < 0)
    {
        perror("Failed to change directory");
        exit(1);
    }

    PRINTF("Starting:\n");
    PRINTF("* workdir: %s\n",  workdir);
    PRINTF("* login_server: %s\n", login_server);
    PRINTF("* map_server: %s\n", map_server);
    PRINTF("* char_server: %s\n", char_server);
    {
        //make sure all possible file descriptors are free for use by the servers
        //if there are file descriptors higher than the max open from before the limit dropped, that's not our problem
        int fd = sysconf(_SC_OPEN_MAX);
        while (--fd > 2)
           if (close(fd) == 0)
               FPRINTF(stderr, "close fd %d\n", fd);
        fd = open("/dev/null", O_RDWR);
        if (fd < 0)
        {
            perror("open /dev/null");
            exit(1);
        }
        dup2(fd, 0);
        dup2(fd, 1);
        close(fd);
    }
    while (1)
    {
        // write stuff to stderr
        timestamp_seconds_buffer timestamp;
        stamp_time(timestamp);

        if (!pid_login)
        {
            pid_login = start_process(login_server);
            FPRINTF(stderr, "[%s] forked login server: %lu\n",
                    timestamp, static_cast<unsigned long>(pid_login));
        }
        if (!pid_char)
        {
            pid_char = start_process(char_server);
            FPRINTF(stderr, "[%s] forked char server: %lu\n",
                    timestamp, static_cast<unsigned long>(pid_char));
        }
        if (!pid_map)
        {
            pid_map = start_process(map_server);
            FPRINTF(stderr, "[%s] forked map server: %lu\n",
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

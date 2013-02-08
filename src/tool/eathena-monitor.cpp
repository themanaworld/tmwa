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
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "../common/cxxstdio.hpp"

#include "../poison.hpp"

#define LOGIN_SERVER "./login-server"
#define MAP_SERVER "./map-server"
#define CHAR_SERVER "./char-server"
#define CONFIG "conf/eathena-monitor.conf"
#define LOGFILE "log/eathena-monitor.log"


static
void SKIP_BLANK(char *& ptr)
{
    while (
        (*ptr == ' ') ||
        (*ptr == '\b') ||
        (*ptr == '\n') ||
        (*ptr == '\r')
    )
        ptr++;
}

static
void GOTO_EQL(char *& ptr) {
    while (
        (*ptr != '\0') &&
        (*ptr != '=') &&
        (*ptr != '\n') &&
        (*ptr != '\r')
    )
        ptr++;
}

static
void GOTO_EOL(char *& ptr) {
    while (
        (*ptr != '\0') &&
        (*ptr != '\n') &&
        (*ptr != '\r')
    )
        ptr++;
}

// initialiized to $HOME/tmwserver
static
const char *workdir;
//the rest are relative to workdir
static
const char *login_server = LOGIN_SERVER;
static
const char *map_server = MAP_SERVER;
static
const char *char_server = CHAR_SERVER;
static
const char *logfile = LOGFILE;
// this variable is hard-coded, but the command-line is checked first
static
const char *config = CONFIG;

static
pid_t pid_login, pid_map, pid_char;

static
const char* make_path(const char* base, const char* path) {
    size_t base_len = strlen(base);
    size_t path_len = strlen(path);
    char* out = (char *)malloc(base_len + 1 + path_len + 1);
    memcpy(out, base, base_len);
    out[base_len] = '/';
    memcpy(out + base_len + 1, path, path_len);
    out[base_len + 1 + path_len] = '\0';
    return out;
}

static
void parse_option(char *name, char *value) {
    if (!strcasecmp(name, "login_server")) {
        login_server = strdup(value);
    } else if (!strcasecmp(name, "map_server")) {
        map_server = strdup(value);
    } else if (!strcasecmp(name, "char_server")) {
        char_server = strdup(value);
    } else if (!strcasecmp(name, "workdir")) {
        workdir = strdup(value);
    } else if (!strcasecmp(name, "logfile")) {
        logfile = strdup(value);
    } else {
        FPRINTF(stderr, "WARNING: ingnoring invalid option '%s' = '%s'\n", name, value);
    }
}

static
void read_config(const char *filename) {
    FILE *input;
    char string[1000];

    if (!(input = fopen(filename,"r")) && !(input = fopen(config, "r"))) {
        perror("Unable to load config file");
        return;
    }

    while (1) {
        if (fgets(string, sizeof(string) - 1, input) == NULL)
            break;
        char *str = string, *name, *value;
        SKIP_BLANK(str);
        string[sizeof(string) - 1] = '\0';
        if (*str == '#')
            continue;
        if (*str == '\0')
            continue;
        name = str;

        GOTO_EQL(str);

        if (*str != '=') {
            continue;
        }

        *str++ = '\0';
        SKIP_BLANK(str);
        value = str;
        GOTO_EOL(str);
        *str = '\0';
        parse_option(name, value);
    }

    fclose(input);
}

static
pid_t start_process(const char *exec) {
    const char *args[2] = {exec, NULL};
    pid_t pid = fork();
    if (pid == -1) {
        FPRINTF(stderr, "Failed to fork");
        return 0;
    }
    if (pid == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
        execv(exec, (char**)args);
#pragma GCC diagnostic pop
        perror("Failed to exec");
        kill(getppid(), SIGABRT);
        exit(1);
    }
    return pid;
}

// Kill all children with the same signal we got, then ourself.
static
void stop_process(int sig) {
    if (pid_map) kill(pid_map, sig);
    if (pid_login) kill(pid_login, sig);
    if (pid_char) kill(pid_char, sig);
    signal(sig, SIG_DFL);
    raise(sig);
}

int main(int argc, char *argv[]) {
    // These are all the signals we are likely to get
    // The shell handles stop/cont
    signal(SIGTERM, stop_process);
    signal(SIGINT, stop_process);
    signal(SIGQUIT, stop_process);
    signal(SIGABRT, stop_process);

    workdir = make_path(getenv("HOME"), "tmwserver");

    read_config(argc>1 ? argv[1] : NULL);

    if (chdir(workdir) < 0) perror("Failed to change directory"), exit(1);

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
        if (fd < 0) perror("open /dev/null"), exit(1);
        dup2(fd, 0);
        dup2(fd, 1);
        close(fd);
    }
    while (1) {
        // write stuff to stderr
        time_t t = time(NULL);
        struct tm *tmp = localtime(&t);
        char timestamp[256];
        strftime(timestamp, sizeof(timestamp), "%F %T", tmp);

        if (!pid_login) {
            pid_login = start_process(login_server);
            FPRINTF(stderr, "[%s] forked login server: %lu\n", timestamp, (unsigned long)pid_login);
        }
        if (!pid_char) {
            pid_char = start_process(char_server);
            FPRINTF(stderr, "[%s] forked char server: %lu\n", timestamp, (unsigned long)pid_char);
        }
        if (!pid_map) {
            pid_map = start_process(map_server);
            FPRINTF(stderr, "[%s] forked map server: %lu\n", timestamp, (unsigned long)pid_map);
        }
        pid_t dead = wait(NULL);
        if (dead < 0) perror("Failed to wait for child"), exit(1);
        if (pid_login == dead) pid_login = 0;
        if (pid_char == dead) pid_char = 0;
        if (pid_map == dead) pid_map = 0;
    }
}

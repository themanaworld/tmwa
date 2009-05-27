/*
Name:    eAthena processes monitor
Author:  Bartosz Waszak <waszi@evil.org.pl>
License: GPL
Compilation:
gcc -o eathena-monitor eathena-monitor.c
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define HOME getenv("HOME")
#define LOGIN_SERVER "login-server"
#define MAP_SERVER "map-server"
#define CHAR_SERVER "char-server"
#define CONFIG "conf/eathena-monitor.conf"
#define LOGFILE "log/eathena-monitor.log"
#define PATH_MAX 4096

#define IS_BLANK(ptr) \
    (((*(ptr)) == ' ') || ((*(ptr)) == '\b') || \
    ((*(ptr)) == '\n') || ((*(ptr)) == '\r'))

#define SKIP_BLANK(ptr) \
    { while (((*(ptr)) == ' ') || ((*(ptr)) == '\b') || \
    ((*(ptr)) == '\n') || ((*(ptr)) == '\r')) ptr++; }

#define GOTO_EQL(ptr) \
    { while (((*(ptr)) != '\0') && ((*(ptr)) != '=') && \
    ((*(ptr)) != '\n') && ((*(ptr)) != '\r')) ptr++; }

#define GOTO_EOL(ptr) \
    { while (((*(ptr)) != '\0') && \
    ((*(ptr)) != '\n') && ((*(ptr)) != '\r')) ptr++; }

char *workdir;
char *login_server;
char *map_server;
char *char_server;
char *config;
char *logfile;
unsigned int interval = 5;
unsigned int pid_login, pid_map, pid_char;
char use_login = 1;

void parse_option (char *name, char *value) {
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
    } else if (!strcasecmp(name, "interval")) {
        interval = atoi(strdup(value));
    } else {
        printf("WARNING: ingnoring invalid options '%s'\n", name);
    }
}

int read_config(char *filename) {
    FILE *input;
    char *str, *base;
    char string[1000];
    char *name;
    char *value;
    int errors = 0;

    if (!(input = fopen(filename,"r")) && !(input = fopen (config, "r"))) {
        fprintf (stderr, "ERROR: Config file doesn't exist (%s and %s), using builtin defaults\n", filename, config);
        return -1;
    }

    while (1) {
        if (fgets (&string[0], sizeof (string) - 1, input) == NULL)
            break;
        str = &string[0];
        SKIP_BLANK(str);
        string[sizeof (string) - 1] = '\0';
        if (*str == '#')
            continue;
        if (*str == '\0')
            continue;
        name = str;

        GOTO_EQL (str);

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

    fclose (input);
    return(0);
}

void start_process(char *exec) {
    pid_t pid;
    pid = fork();
    if (pid == 0) {
    if (!fork()) {
        execl(exec,exec,NULL);
        exit(0);
    }
    exit(0);
    }
    wait(0);
}

void stop_process(int sig) {
    system("killall map-server");
    system("killall login-server");
    system("killall char-server");
    exit(0);
}

int main(int argc, char *argv[]) {
    DIR *procdir;
    FILE *log;

    int fd;

    char pathbuf[PATH_MAX];
    char link[PATH_MAX];
    char timestamp[256];

    struct tm *tmp;
    struct dirent *procdirp;
    struct statfs sfs;

    unsigned int proc_login, proc_map, proc_char;

    time_t t;
    size_t l_size;

    if ( statfs("/proc", &sfs) == -1 ) {
        fprintf(stderr,"ERROR: /proc filesystem is unaccessible\n");
        return(255);
    }

    signal(SIGTERM, stop_process);
    signal(SIGINT, stop_process);

    workdir = (char *) malloc(sizeof(char) * 1024);
    login_server = (char *) malloc(sizeof(char) * 1024);
    char_server = (char *) malloc(sizeof(char) * 1024);
    map_server = (char *) malloc(sizeof(char) * 1024);
    logfile = (char *) malloc(sizeof(char) *  1024);
    config = (char *) malloc(sizeof(char) * 1024);

    sprintf(workdir,"%s/tmwserver",HOME);
    sprintf(login_server,"%s/%s", workdir, LOGIN_SERVER);
    sprintf(map_server,"%s/%s",  workdir, MAP_SERVER);
    sprintf(char_server,"%s/%s", workdir, CHAR_SERVER);
    sprintf(logfile,"%s/%s", workdir,LOGFILE);
    sprintf(config,"%s/%s", workdir, CONFIG);

    read_config(argv[1]);

    chdir(workdir);

    if (strlen(login_server) == 0) use_login = 0;

    printf ("Starting:\n");
    printf ("* interval: %d s\n", interval);
    printf ("* workdir: %s\n",  workdir);
    if (use_login)
        printf ("* login_server: %s\n", login_server);
    else
        printf ("* login_server: (none)\n");
    printf ("* map_server: %s\n", map_server);
    printf ("* char_server: %s\n", char_server);

    if (fork()) {
        exit(0);
    }

    if ((fd = open("/dev/null", O_RDONLY)) != 0) {
        dup2(fd, 0);
        close(fd);
    }

    if ((fd = open("/dev/null", O_WRONLY)) != 1) {
        dup2(fd, 1);
        close(fd);
    }
    dup2(1, 2);

    while (1) {
        if (use_login) proc_login = 0;
        proc_map = 0;
        proc_char = 0;

        if ((procdir = opendir("/proc")) == NULL) {
            fprintf(stderr,"ERROR: Cannot open /proc filesystem\n");
            return(255);
        }

        while ((procdirp = readdir(procdir)) != NULL) {
            if (strtok(procdirp->d_name, "0123456789") == NULL) {
                sprintf(pathbuf, "%s%s%s", "/proc/", procdirp->d_name, "/exe");
                l_size = readlink(pathbuf, link, PATH_MAX);

                if (l_size != -1) {
                    link[l_size] = '\0';
                    if (use_login && !strcmp(link, login_server)) {
                        proc_login = 1;
                        pid_login = (unsigned int) procdirp->d_name;
                    }

                    if (!strcmp(link, char_server)) {
                        proc_char = 1;
                        pid_char = (unsigned int) procdirp->d_name;
                    }

                    if (!strcmp(link, map_server)) {
                        proc_map = 1;
                        pid_map = (unsigned int) procdirp->d_name;
                    }
                }
            }
        }
        closedir(procdir);

        if (!(log = fopen (logfile,"a"))) {
            log = fopen("/tmp/monitor.log","a");
        }

        t = time(NULL);
        tmp = localtime(&t);
        strftime(timestamp, sizeof(timestamp), "%F %X", tmp);

        if (use_login && proc_login == 0) {
            fprintf (log,"[%d][%s] NOTICE: Login server is dead - restarting\n", getpid(), timestamp);
            start_process(login_server);
            sleep(2);
        }
        if (proc_char == 0) {
            fprintf (log,"[%d][%s] NOTICE: Char server is dead - restarting\n", getpid(), timestamp);
            start_process(char_server);
            sleep(2);
        }
        if (proc_map == 0) {
            fprintf (log,"[%d][%s] NOTICE: Map server is dead - restarting\n", getpid(), timestamp);
            start_process(map_server);
            sleep(2);
        }

        fclose(log);
        sleep(interval);
    }
}

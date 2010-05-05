/* To build:
gcc -O2 -m32 -Isrc/common -Isrc/char -Isrc/map -Isrc/login -o marriage-info \
marriage-info.c src/common/socket.o src/common/timer.o src/common/db.o \
src/common/lock.o src/common/malloc.o src/char/int_guild.o \
src/char/int_party.o src/char/int_storage.o src/char/inter.o
*/

#include <stdio.h>
#include <stdlib.h>
#include "login.h"
#include "mmo.h"
#include "char.c"

int mode;
#define MODE_MARRIED	0
#define MODE_SINGLES_F	1
#define MODE_SINGLES_M	2
#define MODE_SINGLES_A	3	

#define CHUNK_SIZE 1024

/* chunked list to represent leaves */
typedef struct {
        int char_id;
        int exp;
        char name[24];
        unsigned char level;
        unsigned char sex;
} char_leaf_t;

/* Chunks are in descending order, but chars are in ascending order */
typedef struct chunklist_node {
        int size;
        char_leaf_t chars[CHUNK_SIZE];
        struct chunklist_node *next;
} chunklist_node_t;

chunklist_node_t *list = NULL;

static void
insert(chunklist_node_t **listp, struct mmo_charstatus *p)
{
        chunklist_node_t *chunk = *listp;
        char_leaf_t *l;

        if ((chunk && chunk->size == CHUNK_SIZE)
            || (!chunk)) {
                chunk = malloc(sizeof(chunklist_node_t));
                chunk->size = 0;
                chunk->next = *listp;
                *listp = chunk;
        }

        l = &chunk->chars[chunk->size++];

        l->char_id = p->char_id;
        l->level = p->base_level;
        l->exp = p->base_exp;
        l->sex = p->sex;
        strcpy(l->name, p->name);
}

static int
compare_leaf(const void *l, const void *r)
{
        return ((char_leaf_t *)l)->char_id - ((char_leaf_t *)r)->char_id;
}

static char_leaf_t *
find(chunklist_node_t *list, int char_id)
{
        if (!list)
                return NULL; /*  fatal error */
        else {
                int start = list->chars[0].char_id;
                int stop = list->chars[list->size - 1].char_id;

                if (char_id >= start && char_id <= stop) { /* in this chunk */
                        char_leaf_t key;
                        key.char_id = char_id;
                        return (char_leaf_t *)
                                bsearch(&key, &list->chars, list->size, sizeof(char_leaf_t),
                                        compare_leaf);
                }
                else find(list->next, char_id);
        }
}


void /* replace blanks with percent signs */
namefrob(char *n)
{
        while (*n++) /*  can't start with a blank */
                if (*n == ' ')
                        *n = '%';
}

/*
--------------------------------------------------------------------------------
 Copied and pasted due to lacking modularity
*/

char account_filename[1024] = "save/account.txt";
int account_id_count = START_ACCOUNT_NUM;

struct auth_dat {
	int account_id, sex;
	char userid[24], pass[24], lastlogin[24];
	int logincount;
	int state;
	char email[40];
	char error_message[20];
	time_t ban_until_time; 
	time_t connect_until_time;
	char last_ip[16];
	char memo[255];
	int account_reg2_num;
	struct global_reg account_reg2[ACCOUNT_REG2_NUM];
} *auth_dat;

int auth_num = 0, auth_max = 0;


/*
 Reading of the accounts database
*/
int mmo_auth_init(void) {
	FILE *fp;
	int account_id, logincount, state, n, i, j, v;
	char line[2048], *p, userid[2048], pass[2048], lastlogin[2048], sex, email[2048], error_message[2048], last_ip[2048], memo[2048];
	time_t ban_until_time;
	time_t connect_until_time;
	char str[2048];
	int GM_count = 0;
	int server_count = 0;

	auth_dat = calloc(sizeof(struct auth_dat) * 256, 1);
	auth_max = 256;

	fp = fopen(account_filename, "r");
	if (fp == NULL) {
		printf("\033[1;31mmmo_auth_init: Accounts file [%s] not found.\033[0m\n", account_filename);
		return 0;
	}

	while(fgets(line, sizeof(line)-1, fp) != NULL) {
		if (line[0] == '/' && line[1] == '/')
			continue;
		line[sizeof(line)-1] = '\0';
		p = line;

		if (((i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t"
		                 "%[^\t]\t%[^\t]\t%ld\t%[^\t]\t%[^\t]\t%ld%n",
		    &account_id, userid, pass, lastlogin, &sex, &logincount, &state,
		    email, error_message, &connect_until_time, last_ip, memo, &ban_until_time, &n)) == 13 && line[n] == '\t') ||
		    ((i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t"
		                 "%[^\t]\t%[^\t]\t%ld\t%[^\t]\t%[^\t]%n",
		    &account_id, userid, pass, lastlogin, &sex, &logincount, &state,
		    email, error_message, &connect_until_time, last_ip, memo, &n)) == 12 && line[n] == '\t')) {
			n = n + 1;

			if (account_id > END_ACCOUNT_NUM) {
				continue;
			}
			userid[23] = '\0';
			remove_control_chars(userid);
			for(j = 0; j < auth_num; j++) {
				if (auth_dat[j].account_id == account_id) {
					break;
				} else if (strcmp(auth_dat[j].userid, userid) == 0) {
					break;
				}
			}
			if (j != auth_num)
				continue;

			if (auth_num >= auth_max) {
				auth_max += 256;
				auth_dat = realloc(auth_dat, sizeof(struct auth_dat) * auth_max);
			}

			memset(&auth_dat[auth_num], '\0', sizeof(struct auth_dat));

			auth_dat[auth_num].account_id = account_id;

			strncpy(auth_dat[auth_num].userid, userid, 24);

			pass[23] = '\0';
			remove_control_chars(pass);
			strncpy(auth_dat[auth_num].pass, pass, 24);

			lastlogin[23] = '\0';
			remove_control_chars(lastlogin);
			strncpy(auth_dat[auth_num].lastlogin, lastlogin, 24);

			auth_dat[auth_num].sex = (sex == 'S' || sex == 's') ? 2 : (sex == 'M' || sex == 'm');

			if (logincount >= 0)
				auth_dat[auth_num].logincount = logincount;
			else
				auth_dat[auth_num].logincount = 0;

			if (state > 255)
				auth_dat[auth_num].state = 100;
			else if (state < 0)
				auth_dat[auth_num].state = 0;
			else
				auth_dat[auth_num].state = state;

			if (e_mail_check(email) == 0) {
				strncpy(auth_dat[auth_num].email, "a@a.com", 40);
			} else {
				remove_control_chars(email);
				strncpy(auth_dat[auth_num].email, email, 40);
			}

			error_message[19] = '\0';
			remove_control_chars(error_message);
			if (error_message[0] == '\0' || state != 7) {
				strncpy(auth_dat[auth_num].error_message, "-", 20);
			} else {
				strncpy(auth_dat[auth_num].error_message, error_message, 20);
			}

			if (i == 13)
				auth_dat[auth_num].ban_until_time = ban_until_time;
			else
				auth_dat[auth_num].ban_until_time = 0;

			auth_dat[auth_num].connect_until_time = connect_until_time;

			last_ip[15] = '\0';
			remove_control_chars(last_ip);
			strncpy(auth_dat[auth_num].last_ip, last_ip, 16);

			memo[254] = '\0';
			remove_control_chars(memo);
			strncpy(auth_dat[auth_num].memo, memo, 255);

			for(j = 0; j < ACCOUNT_REG2_NUM; j++) {
				p += n;
				if (sscanf(p, "%[^\t,],%d %n", str, &v, &n) != 2) {
					if (p[0] == ',' && sscanf(p, ",%d %n", &v, &n) == 1) {
						j--;
						continue;
					} else
						break;
				}
				str[31] = '\0';
				remove_control_chars(str);
				strncpy(auth_dat[auth_num].account_reg2[j].str, str, 32);
				auth_dat[auth_num].account_reg2[j].value = v;
			}
			auth_dat[auth_num].account_reg2_num = j;

			if (isGM(account_id) > 0)
				GM_count++;
			if (auth_dat[auth_num].sex == 2)
				server_count++;

			auth_num++;
			if (account_id >= account_id_count)
				account_id_count = account_id + 1;

		} else if ((i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t%n",
		           &account_id, userid, pass, lastlogin, &sex, &logincount, &state, &n)) >= 5) {
			if (account_id > END_ACCOUNT_NUM) {
				continue;
			}
			userid[23] = '\0';
			remove_control_chars(userid);
			for(j = 0; j < auth_num; j++) {
				if (auth_dat[j].account_id == account_id) {
					break;
				} else if (strcmp(auth_dat[j].userid, userid) == 0) {
					break;
				}
			}
			if (j != auth_num)
				continue;

			if (auth_num >= auth_max) {
				auth_max += 256;
				auth_dat = realloc(auth_dat, sizeof(struct auth_dat) * auth_max);
			}

			memset(&auth_dat[auth_num], '\0', sizeof(struct auth_dat));

			auth_dat[auth_num].account_id = account_id;

			strncpy(auth_dat[auth_num].userid, userid, 24);

			pass[23] = '\0';
			remove_control_chars(pass);
			strncpy(auth_dat[auth_num].pass, pass, 24);

			lastlogin[23] = '\0';
			remove_control_chars(lastlogin);
			strncpy(auth_dat[auth_num].lastlogin, lastlogin, 24);

			auth_dat[auth_num].sex = (sex == 'S' || sex == 's') ? 2 : (sex == 'M' || sex == 'm');

			if (i >= 6) {
				if (logincount >= 0)
					auth_dat[auth_num].logincount = logincount;
				else
					auth_dat[auth_num].logincount = 0;
			} else
				auth_dat[auth_num].logincount = 0;

			if (i >= 7) {
				if (state > 255)
					auth_dat[auth_num].state = 100;
				else if (state < 0)
					auth_dat[auth_num].state = 0;
				else
					auth_dat[auth_num].state = state;
			} else
				auth_dat[auth_num].state = 0;

			strncpy(auth_dat[auth_num].email, "a@a.com", 40);
			strncpy(auth_dat[auth_num].error_message, "-", 20);
			auth_dat[auth_num].ban_until_time = 0;
			auth_dat[auth_num].connect_until_time = 0;
			strncpy(auth_dat[auth_num].last_ip, "-", 16);
			strncpy(auth_dat[auth_num].memo, "-", 255);

			for(j = 0; j < ACCOUNT_REG2_NUM; j++) {
				p += n;
				if (sscanf(p, "%[^\t,],%d %n", str, &v, &n) != 2) {
					if (p[0] == ',' && sscanf(p, ",%d %n", &v, &n) == 1) {
						j--;
						continue;
					} else
						break;
				}
				str[31] = '\0';
				remove_control_chars(str);
				strncpy(auth_dat[auth_num].account_reg2[j].str, str, 32);
				auth_dat[auth_num].account_reg2[j].value = v;
			}
			auth_dat[auth_num].account_reg2_num = j;

			if (isGM(account_id) > 0)
				GM_count++;
			if (auth_dat[auth_num].sex == 2)
				server_count++;

			auth_num++;
			if (account_id >= account_id_count)
				account_id_count = account_id + 1;

		} else {
			i = 0;
			if (sscanf(line, "%d\t%%newid%%\n%n", &account_id, &i) == 1 &&
			    i > 0 && account_id > account_id_count)
				account_id_count = account_id;
		}
	}
	fclose(fp);

	if (auth_num == 0) {
		sprintf(line, "No account found in %s.", account_filename);
	} else {
		if (auth_num == 1) {
			sprintf(line, "1 account read in %s,", account_filename);
		} else {
			sprintf(line, "%d accounts read in %s,", auth_num, account_filename);
		}
		if (GM_count == 0) {
			sprintf(str, "%s of which is no GM account and", line);
		} else if (GM_count == 1) {
			sprintf(str, "%s of which is 1 GM account and", line);
		} else {
			sprintf(str, "%s of which is %d GM accounts and", line, GM_count);
		}
		if (server_count == 0) {
			sprintf(line, "%s no server account ('S').", str);
		} else if (server_count == 1) {
			sprintf(line, "%s 1 server account ('S').", str);
		} else {
			sprintf(line, "%s %d server accounts ('S').", str, server_count);
		}
	}

	return 0;
}

/*--------------------------------------------------------------------------------*/


void
mmo_check_dumpworthy(struct mmo_charstatus *p)
{
        int i;
        namefrob(p->name);

        for (i = 0; i < auth_num; i++)
                if (auth_dat[i].account_id == p->account_id) {
                        p->sex = auth_dat[i].sex;
                        break;
                }

        if (p->partner_id) {
                if (mode != MODE_MARRIED)
                        return;

                if (p->partner_id < p->char_id) {
                        char_leaf_t *partner = find(list, p->partner_id);
                        if (!partner)
                                fprintf(stderr, "Char %d (%s)'s partner (%d) no longer available\n",
                                        p->char_id,
                                        p->name,
                                        p->partner_id);
                        else
                                printf ("%d\t%d\t%s\t%s\t%d\t%s\t%s\t%d--%d,%d\n",
                                        p->base_level + partner->level,
                                        p->base_exp + partner->exp,
                                        p->name, p->sex? "M" : "F", p->base_level,
                                        partner->name, partner->sex? "M" : "F", partner->level);
                } else {
                        insert(&list, p);
                }
        } else if (p->sex == (mode - 1))
                        printf("%d\t%d\t%s\n", p->base_level, p->base_exp, p->name);
        else if (mode == MODE_SINGLES_A)
                        printf("%d\t%d\t%s\t%s\n", p->base_level, p->base_exp, p->name, p->sex? "M" : "F");
}

int mmo_char_dump()
{
        char line[965536];
        int ret;
	struct mmo_charstatus char_dat;
        FILE *ifp,*ofp;

	ifp=stdin;
        while(fgets(line,65535,ifp)){
                memset(&char_dat,0,sizeof(struct mmo_charstatus));
                ret=mmo_char_fromstr(line,&char_dat);
                if(ret){
                        mmo_check_dumpworthy(&char_dat);
                }
        }
        fcloseall();
        return 0;
}


int init(char *mode_s)
{
        if (!strcmp(mode_s, "-c"))
                mode = MODE_MARRIED;
        else if (!strcmp(mode_s, "-f"))
                mode = MODE_SINGLES_F;
        else if (!strcmp(mode_s, "-m"))
                mode = MODE_SINGLES_M;
        else if (!strcmp(mode_s, "-s"))
                mode = MODE_SINGLES_A;
        else {
                fprintf(stderr, "Unknown mode `%s'\n", mode_s);
                return 1;
        }
        return 0;
}

int main(int argc,char *argv[])
{
        mmo_auth_init();

	if(argc < 2) {
		printf("Usage: %s <mode>\n", argv[0]);
                printf("Where <mode> is one of -c (couples), -s (singles), -f (female singles), -m (male singles)\n", argv[0]);
		exit(0);
	}
        if (init(argv[1]))
                return 1;

	mmo_char_dump();

	return 0;
}


/* satisfy linker */
void set_termfunc(void (*termfunc)(void)) {};

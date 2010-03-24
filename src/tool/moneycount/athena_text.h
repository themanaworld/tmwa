#ifndef ATHENA_TEXT_H
#define ATHENA_TEXT_H
#include "mmo.h"

int mmo_char_fromstr (char *str, struct mmo_charstatus *p);

int accreg_fromstr (char *str, struct accreg *reg);

#endif

#include "athena_text.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmo.h"

//-------------------------------------------------------------------------
// Function to set the character from the line (at read of characters file)
//-------------------------------------------------------------------------
int mmo_char_fromstr (char *str, struct mmo_charstatus *p)
{
    int  tmp_int[256];
    int  set, next, len, i;

    // initilialise character
    memset (p, '\0', sizeof (struct mmo_charstatus));

    // If it's not char structure of version 1008 and after
    if ((set = sscanf (str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d" "\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d" "\t%[^,],%d,%d\t%[^,],%d,%d,%d%n", &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name,  //
                       &tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6], &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12], &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18], &tmp_int[19], &tmp_int[20], &tmp_int[21], &tmp_int[22], &tmp_int[23],   //
                       &tmp_int[24], &tmp_int[25], &tmp_int[26], &tmp_int[27], &tmp_int[28], &tmp_int[29], &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34], p->last_point.map, &tmp_int[35], &tmp_int[36], //
                       p->save_point.map, &tmp_int[37], &tmp_int[38],
                       &tmp_int[39], &next)) != 43)
    {
        tmp_int[39] = 0;        // partner id
        // If not char structure from version 384 to 1007
        if ((set = sscanf (str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d" "\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d" "\t%[^,],%d,%d\t%[^,],%d,%d%n", &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
                           &tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6], &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12], &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18], &tmp_int[19], &tmp_int[20], &tmp_int[21], &tmp_int[22], &tmp_int[23],   //
                           &tmp_int[24], &tmp_int[25], &tmp_int[26], &tmp_int[27], &tmp_int[28], &tmp_int[29], &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34], p->last_point.map, &tmp_int[35], &tmp_int[36], //
                           p->save_point.map, &tmp_int[37], &tmp_int[38],
                           &next)) != 42)
        {
            // It's char structure of a version before 384
            tmp_int[26] = 0;    // pet id
            set = sscanf (str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d" "\t%d,%d,%d\t%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d" "\t%[^,],%d,%d\t%[^,],%d,%d%n", &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
                          &tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6], &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12], &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18], &tmp_int[19], &tmp_int[20], &tmp_int[21], &tmp_int[22], &tmp_int[23],    //
                          &tmp_int[24], &tmp_int[25],   //
                          &tmp_int[27], &tmp_int[28], &tmp_int[29], &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34], p->last_point.map, &tmp_int[35], &tmp_int[36],    //
                          p->save_point.map, &tmp_int[37], &tmp_int[38],
                          &next);
            set += 2;
            //printf("char: old char data ver.1\n");
            // Char structure of version 1007 or older
        }
        else
        {
            set++;
            //printf("char: old char data ver.2\n");
        }
        // Char structure of version 1008+
    }
    else
    {
        //printf("char: new char data ver.3\n");
    }
    if (set != 43)
        return 0;

    p->char_id = tmp_int[0];
    p->account_id = tmp_int[1];
    p->char_num = tmp_int[2];
    p->classb = tmp_int[3];
    p->base_level = tmp_int[4];
    p->job_level = tmp_int[5];
    p->base_exp = tmp_int[6];
    p->job_exp = tmp_int[7];
    p->zeny = tmp_int[8];
    p->hp = tmp_int[9];
    p->max_hp = tmp_int[10];
    p->sp = tmp_int[11];
    p->max_sp = tmp_int[12];
    p->str = tmp_int[13];
    p->agi = tmp_int[14];
    p->vit = tmp_int[15];
    p->int_ = tmp_int[16];
    p->dex = tmp_int[17];
    p->luk = tmp_int[18];
    p->status_point = tmp_int[19];
    p->skill_point = tmp_int[20];
    p->option = tmp_int[21];
    p->karma = tmp_int[22];
    p->manner = tmp_int[23];
    p->party_id = tmp_int[24];
    p->guild_id = tmp_int[25];
//  p->pet_id = tmp_int[26];
    p->hair = tmp_int[27];
    p->hair_color = tmp_int[28];
    p->clothes_color = tmp_int[29];
    p->weapon = tmp_int[30];
    p->shield = tmp_int[31];
    p->head_top = tmp_int[32];
    p->head_mid = tmp_int[33];
    p->head_bottom = tmp_int[34];
    p->last_point.x = tmp_int[35];
    p->last_point.y = tmp_int[36];
    p->save_point.x = tmp_int[37];
    p->save_point.y = tmp_int[38];
    p->partner_id = tmp_int[39];

    if (str[next] == '\n' || str[next] == '\r')
        return 1;               // ?V?K?f?[?^

    next++;

    for (i = 0; str[next] && str[next] != '\t'; i++)
    {
        if (sscanf
            (str + next, "%[^,],%d,%d%n", p->memo_point[i].map, &tmp_int[0],
             &tmp_int[1], &len) != 3)
            return -3;
        p->memo_point[i].x = tmp_int[0];
        p->memo_point[i].y = tmp_int[1];
        next += len;
        if (str[next] == ' ')
            next++;
    }

    next++;

    for (i = 0; str[next] && str[next] != '\t'; i++)
    {
        if (sscanf (str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
                    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
                    &tmp_int[4], &tmp_int[5], &tmp_int[6],
                    &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10],
                    &tmp_int[11], &len) == 12)
        {
            // do nothing, it's ok
        }
        else if (sscanf (str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
                         &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
                         &tmp_int[4], &tmp_int[5], &tmp_int[6],
                         &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10],
                         &len) == 11)
        {
            tmp_int[11] = 0;    // broken doesn't exist in this version -> 0
        }
        else                    // invalid structure
            return -4;
        p->inventory[i].id = tmp_int[0];
        p->inventory[i].nameid = tmp_int[1];
        p->inventory[i].amount = tmp_int[2];
        p->inventory[i].equip = tmp_int[3];
        p->inventory[i].identify = tmp_int[4];
        p->inventory[i].refine = tmp_int[5];
        p->inventory[i].attribute = tmp_int[6];
        p->inventory[i].card[0] = tmp_int[7];
        p->inventory[i].card[1] = tmp_int[8];
        p->inventory[i].card[2] = tmp_int[9];
        p->inventory[i].card[3] = tmp_int[10];
        p->inventory[i].broken = tmp_int[11];
        next += len;
        if (str[next] == ' ')
            next++;
    }

    next++;

    for (i = 0; str[next] && str[next] != '\t'; i++)
    {
        if (sscanf (str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
                    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
                    &tmp_int[4], &tmp_int[5], &tmp_int[6],
                    &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10],
                    &tmp_int[11], &len) == 12)
        {
            // do nothing, it's ok
        }
        else if (sscanf (str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
                         &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
                         &tmp_int[4], &tmp_int[5], &tmp_int[6],
                         &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10],
                         &len) == 11)
        {
            tmp_int[11] = 0;    // broken doesn't exist in this version -> 0
        }
        else                    // invalid structure
            return -5;
        p->cart[i].id = tmp_int[0];
        p->cart[i].nameid = tmp_int[1];
        p->cart[i].amount = tmp_int[2];
        p->cart[i].equip = tmp_int[3];
        p->cart[i].identify = tmp_int[4];
        p->cart[i].refine = tmp_int[5];
        p->cart[i].attribute = tmp_int[6];
        p->cart[i].card[0] = tmp_int[7];
        p->cart[i].card[1] = tmp_int[8];
        p->cart[i].card[2] = tmp_int[9];
        p->cart[i].card[3] = tmp_int[10];
        p->cart[i].broken = tmp_int[11];
        next += len;
        if (str[next] == ' ')
            next++;
    }

    next++;

    for (i = 0; str[next] && str[next] != '\t'; i++)
    {
        if (sscanf (str + next, "%d,%d%n", &tmp_int[0], &tmp_int[1], &len) !=
            2)
            return -6;
        p->skill[tmp_int[0]].id = tmp_int[0];
        p->skill[tmp_int[0]].lv = tmp_int[1] & 0xffff;
        p->skill[tmp_int[0]].flags = ((tmp_int[1] >> 16) & 0xffff);
        next += len;
        if (str[next] == ' ')
            next++;
    }

    next++;

    for (i = 0;
         str[next] && str[next] != '\t' && str[next] != '\n'
         && str[next] != '\r'; i++)
    {                           // global_reg?????O??athena.txt????^????'\n'?`?F?b?N
        if (sscanf
            (str + next, "%[^,],%d%n", p->global_reg[i].str,
             &p->global_reg[i].value, &len) != 2)
        {
            // because some scripts are not correct, the str can be "". So, we must check that.
            // If it's, we must not refuse the character, but just this REG value.
            // Character line will have something like: nov_2nd_cos,9 ,9 nov_1_2_cos_c,1 (here, ,9 is not good)
            if (str[next] == ','
                && sscanf (str + next, ",%d%n", &p->global_reg[i].value,
                           &len) == 1)
                i--;
            else
                return -7;
        }
        next += len;
        if (str[next] == ' ')
            next++;
    }
    p->global_reg_num = i;

    return 1;
}

int accreg_fromstr (char *str, struct accreg *reg)
{
    int  j, v, n;
    char buf[128];
    const char *p = str;

     if (sscanf (p, "%d\t%n", &reg->account_id, &n) != 1
         || reg->account_id <= 0)
         return 0;

     for (j = 0, p += n; j < ACCOUNT_REG_NUM; j++, p += n)
     {
         if (sscanf (p, "%[^,],%d %n", buf, &v, &n) != 2)
             break;
         memcpy (reg->reg[j].str, buf, 32);
         reg->reg[j].value = v;
     }
     reg->reg_num = j;

     return 1;
}


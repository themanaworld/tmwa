#include "magic-expr.h"
#include "magic-expr-eval.h"
#include "itemdb.h"
#include <math.h>

#define IS_SOLID(c) ((c) == 1 || (c) == 5)

int map_is_solid (int m, int x, int y)
{
    return (IS_SOLID (map_getcell (m, x, y)));
}

#undef IS_SOLID

static void free_area (area_t * area)
{
    if (!area)
        return;

    switch (area->ty)
    {
        case AREA_UNION:
            free_area (area->a.a_union[0]);
            free_area (area->a.a_union[1]);
            break;
        default:
            break;
    }

    free (area);
}

static area_t *dup_area (area_t * area)
{
    area_t *retval = malloc (sizeof (area_t));
    *retval = *area;

    switch (area->ty)
    {
        case AREA_UNION:
            retval->a.a_union[0] = dup_area (retval->a.a_union[0]);
            retval->a.a_union[1] = dup_area (retval->a.a_union[1]);
            break;
        default:
            break;
    }

    return retval;
}

void magic_copy_var (val_t * dest, val_t * src)
{
    *dest = *src;

    switch (dest->ty)
    {
        case TY_STRING:
            dest->v.v_string = strdup (dest->v.v_string);
            break;
        case TY_AREA:
            dest->v.v_area = dup_area (dest->v.v_area);
            break;
        default:
            break;
    }

}

void magic_clear_var (val_t * v)
{
    switch (v->ty)
    {
        case TY_STRING:
            free (v->v.v_string);
            break;
        case TY_AREA:
            free_area (v->v.v_area);
            break;
        default:
            break;
    }
}

static char *show_entity (entity_t * entity)
{
    switch (entity->type)
    {
        case BL_PC:
            return ((struct map_session_data *) entity)->status.name;
        case BL_NPC:
            return ((struct npc_data *) entity)->name;
        case BL_MOB:
            return ((struct mob_data *) entity)->name;
        case BL_ITEM:
            /* Sorry about this one... */
            return ((struct item_data
                     *) (&((struct flooritem_data *) entity)->
                         item_data))->name;
        case BL_SKILL:
            return "%skill";
        case BL_SPELL:
            return "%invocation(ERROR:this-should-not-be-an-entity)";
        default:
            return "%unknown-entity";
    }
}

static void stringify (val_t * v, int within_op)
{
    static char *dirs[8] =
        { "south", "south-west", "west", "north-west", "north", "north-east",
        "east", "south-east"
    };
    char *buf;

    switch (v->ty)
    {
        case TY_UNDEF:
            buf = strdup ("UNDEF");
            break;

        case TY_INT:
            buf = malloc (32);
            sprintf (buf, "%i", v->v.v_int);
            break;

        case TY_STRING:
            return;

        case TY_DIR:
            buf = strdup (dirs[v->v.v_int]);
            break;

        case TY_ENTITY:
            buf = strdup (show_entity (v->v.v_entity));
            break;

        case TY_LOCATION:
            buf = malloc (128);
            sprintf (buf, "<\"%s\", %d, %d>", map[v->v.v_location.m].name,
                     v->v.v_location.x, v->v.v_location.y);
            break;

        case TY_AREA:
            buf = strdup ("%area");
            free_area (v->v.v_area);
            break;

        case TY_SPELL:
            buf = strdup (v->v.v_spell->name);
            break;

        case TY_INVOCATION:
        {
            invocation_t *invocation = within_op
                ? v->v.v_invocation : (invocation_t *) map_id2bl (v->v.v_int);
            buf = strdup (invocation->spell->name);
        }
            break;

        default:
            fprintf (stderr, "[magic] INTERNAL ERROR: Cannot stringify %d\n",
                     v->ty);
            return;
    }

    v->v.v_string = buf;
    v->ty = TY_STRING;
}

static void intify (val_t * v)
{
    if (v->ty == TY_INT)
        return;

    magic_clear_var (v);
    v->ty = TY_INT;
    v->v.v_int = 1;
}

area_t *area_new (int ty)
{
    area_t *retval = (area_t *) aCalloc (sizeof (area_t), 1);
    retval->ty = ty;
    return retval;
}

area_t *area_union (area_t * area, area_t * other_area)
{
    area_t *retval = area_new (AREA_UNION);
    retval->a.a_union[0] = area;
    retval->a.a_union[1] = other_area;
    retval->size = area->size + other_area->size;   /* Assume no overlap */
    return retval;
}

/**
 * Turns location into area, leaves other types untouched
 */
static void make_area (val_t * v)
{
    if (v->ty == TY_LOCATION)
    {
        area_t *a = malloc (sizeof (area_t));
        v->ty = TY_AREA;
        a->ty = AREA_LOCATION;
        a->a.a_loc = v->v.v_location;
        v->v.v_area = a;
    }
}

static void make_location (val_t * v)
{
    if (v->ty == TY_AREA && v->v.v_area->ty == AREA_LOCATION)
    {
        location_t location = v->v.v_area->a.a_loc;
        free_area (v->v.v_area);
        v->ty = TY_LOCATION;
        v->v.v_location = location;
    }
}

static void make_spell (val_t * v)
{
    if (v->ty == TY_INVOCATION)
    {
        invocation_t *invoc = v->v.v_invocation;    //(invocation_t *) map_id2bl(v->v.v_int);
        if (!invoc)
            v->ty = TY_FAIL;
        else
        {
            v->ty = TY_SPELL;
            v->v.v_spell = invoc->spell;
        }
    }
}

static int fun_add (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (TY (0) == TY_INT && TY (1) == TY_INT)
    {
        /* Integer addition */
        RESULTINT = ARGINT (0) + ARGINT (1);
        result->ty = TY_INT;
    }
    else if (ARG_MAY_BE_AREA (0) && ARG_MAY_BE_AREA (1))
    {
        /* Area union */
        make_area (&args[0]);
        make_area (&args[1]);
        RESULTAREA = area_union (ARGAREA (0), ARGAREA (1));
        ARGAREA (0) = NULL;
        ARGAREA (1) = NULL;
        result->ty = TY_AREA;
    }
    else
    {
        /* Anything else -> string concatenation */
        stringify (&args[0], 1);
        stringify (&args[1], 1);
        /* Yes, we could speed this up. */
        RESULTSTR =
            (char *) malloc (1 + strlen (ARGSTR (0)) + strlen (ARGSTR (1)));
        strcpy (RESULTSTR, ARGSTR (0));
        strcat (RESULTSTR, ARGSTR (1));
        result->ty = TY_STRING;
    }
    return 0;
}

static int fun_sub (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGINT (0) - ARGINT (1);
    return 0;
}

static int fun_mul (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGINT (0) * ARGINT (1);
    return 0;
}

static int fun_div (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (!ARGINT (1))
        return 1;               /* division by zero */
    RESULTINT = ARGINT (0) / ARGINT (1);
    return 0;
}

static int fun_mod (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (!ARGINT (1))
        return 1;               /* division by zero */
    RESULTINT = ARGINT (0) % ARGINT (1);
    return 0;
}

static int fun_or (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGINT (0) || ARGINT (1);
    return 0;
}

static int fun_and (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGINT (0) && ARGINT (1);
    return 0;
}

static int fun_not (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = !ARGINT (0);
    return 0;
}

static int fun_neg (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ~ARGINT (0);
    return 0;
}

static int fun_gte (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (TY (0) == TY_STRING || TY (1) == TY_STRING)
    {
        stringify (&args[0], 1);
        stringify (&args[1], 1);
        RESULTINT = strcmp (ARGSTR (0), ARGSTR (1)) >= 0;
    }
    else
    {
        intify (&args[0]);
        intify (&args[1]);
        RESULTINT = ARGINT (0) >= ARGINT (1);
    }
    return 0;
}

static int fun_gt (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (TY (0) == TY_STRING || TY (1) == TY_STRING)
    {
        stringify (&args[0], 1);
        stringify (&args[1], 1);
        RESULTINT = strcmp (ARGSTR (0), ARGSTR (1)) > 0;
    }
    else
    {
        intify (&args[0]);
        intify (&args[1]);
        RESULTINT = ARGINT (0) > ARGINT (1);
    }
    return 0;
}

static int fun_eq (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (TY (0) == TY_STRING || TY (1) == TY_STRING)
    {
        stringify (&args[0], 1);
        stringify (&args[1], 1);
        RESULTINT = strcmp (ARGSTR (0), ARGSTR (1)) == 0;
    }
    else if (TY (0) == TY_DIR && TY (1) == TY_DIR)
        RESULTINT = ARGDIR (0) == ARGDIR (1);
    else if (TY (0) == TY_ENTITY && TY (1) == TY_ENTITY)
        RESULTINT = ARGENTITY (0) == ARGENTITY (1);
    else if (TY (0) == TY_LOCATION && TY (1) == TY_LOCATION)
        RESULTINT = (ARGLOCATION (0).x == ARGLOCATION (1).x
                     && ARGLOCATION (0).y == ARGLOCATION (1).y
                     && ARGLOCATION (0).m == ARGLOCATION (1).m);
    else if (TY (0) == TY_AREA && TY (1) == TY_AREA)
        RESULTINT = ARGAREA (0) == ARGAREA (1); /* Probably not that great an idea... */
    else if (TY (0) == TY_SPELL && TY (1) == TY_SPELL)
        RESULTINT = ARGSPELL (0) == ARGSPELL (1);
    else if (TY (0) == TY_INVOCATION && TY (1) == TY_INVOCATION)
        RESULTINT = ARGINVOCATION (0) == ARGINVOCATION (1);
    else
    {
        intify (&args[0]);
        intify (&args[1]);
        RESULTINT = ARGINT (0) == ARGINT (1);
    }
    return 0;
}

static int fun_bitand (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGINT (0) & ARGINT (1);
    return 0;
}

static int fun_bitor (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGINT (0) | ARGINT (1);
    return 0;
}

static int fun_bitxor (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGINT (0) ^ ARGINT (1);
    return 0;
}

static int fun_bitshl (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGINT (0) << ARGINT (1);
    return 0;
}

static int fun_bitshr (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGINT (0) >> ARGINT (1);
    return 0;
}

static int fun_max (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = MAX (ARGINT (0), ARGINT (1));
    return 0;
}

static int fun_min (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = MIN (ARGINT (0), ARGINT (1));
    return 0;
}

static int
fun_if_then_else (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (ARGINT (0))
        magic_copy_var (result, &args[1]);
    else
        magic_copy_var (result, &args[2]);
    return 0;
}

void
magic_area_rect (int *m, int *x, int *y, int *width, int *height,
                 area_t * area)
{
    switch (area->ty)
    {
        case AREA_UNION:
            break;

        case AREA_LOCATION:
            *m = area->a.a_loc.m;
            *x = area->a.a_loc.x;
            *y = area->a.a_loc.y;
            *width = 1;
            *height = 1;
            break;

        case AREA_RECT:
            *m = area->a.a_rect.loc.m;
            *x = area->a.a_rect.loc.x;
            *y = area->a.a_rect.loc.y;
            *width = area->a.a_rect.width;
            *height = area->a.a_rect.height;
            break;

        case AREA_BAR:
        {
            int  tx = area->a.a_bar.loc.x;
            int  ty = area->a.a_bar.loc.y;
            int  twidth = area->a.a_bar.width;
            int  tdepth = area->a.a_bar.width;
            *m = area->a.a_bar.loc.m;

            switch (area->a.a_bar.dir)
            {
                case DIR_S:
                    *x = tx - twidth;
                    *y = ty;
                    *width = twidth * 2 + 1;
                    *height = tdepth;
                    break;

                case DIR_W:
                    *x = tx - tdepth;
                    *y = ty - twidth;
                    *width = tdepth;
                    *height = twidth * 2 + 1;
                    break;

                case DIR_N:
                    *x = tx - twidth;
                    *y = ty - tdepth;
                    *width = twidth * 2 + 1;
                    *height = tdepth;
                    break;

                case DIR_E:
                    *x = tx;
                    *y = ty - twidth;
                    *width = tdepth;
                    *height = twidth * 2 + 1;
                    break;

                default:
                    fprintf (stderr,
                             "Error: Trying to compute area of NE/SE/NW/SW-facing bar");
                    *x = tx;
                    *y = ty;
                    *width = *height = 1;
            }
            break;
        }
    }
}

int magic_location_in_area (int m, int x, int y, area_t * area)
{
    switch (area->ty)
    {
        case AREA_UNION:
            return magic_location_in_area (m, x, y, area->a.a_union[0])
                || magic_location_in_area (m, x, y, area->a.a_union[1]);
        case AREA_LOCATION:
        case AREA_RECT:
        case AREA_BAR:
        {
            int  am;
            int  ax, ay, awidth, aheight;
            magic_area_rect (&am, &ax, &ay, &awidth, &aheight, area);
            return (am == m
                    && (x >= ax) && (y >= ay)
                    && (x < ax + awidth) && (y < ay + aheight));
        }
        default:
            fprintf (stderr, "INTERNAL ERROR: Invalid area\n");
            return 0;
    }
}

static int fun_is_in (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = magic_location_in_area (ARGLOCATION (0).m,
                                        ARGLOCATION (0).x,
                                        ARGLOCATION (0).y, ARGAREA (1));
    return 0;
}

static int fun_skill (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (ETY (0) != BL_PC
        || ARGINT (1) < 0
        || ARGINT (1) >= MAX_SKILL
        || ARGPC (0)->status.skill[ARGINT (1)].id != ARGINT (1))
        RESULTINT = 0;
    else
        RESULTINT = ARGPC (0)->status.skill[ARGINT (1)].lv;
    return 0;
}

static int
fun_has_shroud (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = (ETY (0) == BL_PC && ARGPC (0)->state.shroud_active);
    return 0;
}

#define BATTLE_GETTER(name) static int fun_get_##name(env_t *env, int args_nr, val_t *result, val_t *args) { RESULTINT = battle_get_##name(ARGENTITY(0)); return 0; }

BATTLE_GETTER (str);
BATTLE_GETTER (agi);
BATTLE_GETTER (vit);
BATTLE_GETTER (dex);
BATTLE_GETTER (luk);
BATTLE_GETTER (int);
BATTLE_GETTER (lv);
BATTLE_GETTER (hp);
BATTLE_GETTER (mdef);
BATTLE_GETTER (def);
BATTLE_GETTER (max_hp);
BATTLE_GETTER (dir);

#define MMO_GETTER(name) static int fun_get_##name(env_t *env, int args_nr, val_t *result, val_t *args) {	\
                if (ETY(0) == BL_PC)								\
                	RESULTINT = ARGPC(0)->status.name;					\
		else										\
		        RESULTINT = 0;								\
		return 0; }

MMO_GETTER (sp);
MMO_GETTER (max_sp);

static int
fun_name_of (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (TY (0) == TY_ENTITY)
    {
        RESULTSTR = strdup (show_entity (ARGENTITY (0)));
        return 0;
    }
    else if (TY (0) == TY_SPELL)
    {
        RESULTSTR = strdup (ARGSPELL (0)->name);
        return 0;
    }
    else if (TY (0) == TY_INVOCATION)
    {
        RESULTSTR = strdup (ARGINVOCATION (0)->spell->name);
        return 0;
    }
    return 1;
}

/* [Freeyorp] I'm putting this one in as name_of seems to have issues with summoned or spawned mobs. */
static int
fun_mob_id (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (ETY (0) != BL_MOB) return 1;
    RESULTINT = ((struct mob_data *) (ARGENTITY(0)))->class;
    return 0;
}

#define COPY_LOCATION(dest, src) (dest).x = (src).x; (dest).y = (src).y; (dest).m = (src).m;

static int
fun_location (env_t * env, int args_nr, val_t * result, val_t * args)
{
    COPY_LOCATION (RESULTLOCATION, *(ARGENTITY (0)));
    return 0;
}

static int fun_random (env_t * env, int args_nr, val_t * result, val_t * args)
{
    int  delta = ARGINT (0);
    if (delta < 0)
        delta = -delta;
    if (delta == 0)
    {
        RESULTINT = 0;
        return 0;
    }
    RESULTINT = MRAND (delta);

    if (ARGINT (0) < 0)
        RESULTINT = -RESULTINT;
    return 0;
}

static int
fun_random_dir (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (ARGINT (0))
        RESULTDIR = mt_random () & 0x7;
    else
        RESULTDIR = (mt_random () & 0x3) * 2;
    return 0;
}

static int
fun_hash_entity (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGENTITY (0)->id;
    return 0;
}

int                             // ret -1: not a string, ret 1: no such item, ret 0: OK
magic_find_item (val_t * args, int index, struct item *item, int *stackable)
{
    struct item_data *item_data;
    int  must_add_sequentially;

    if (TY (index) == TY_INT)
        item_data = itemdb_exists (ARGINT (index));
    else if (TY (index) == TY_STRING)
        item_data = itemdb_searchname (ARGSTR (index));
    else
        return -1;

    if (!item_data)
        return 1;

    must_add_sequentially = (item_data->type == 4 || item_data->type == 5 || item_data->type == 7 || item_data->type == 8); /* Very elegant. */

    if (stackable)
        *stackable = !must_add_sequentially;

    memset (item, 0, sizeof (struct item));
    item->nameid = item_data->nameid;
    item->identify = 1;

    return 0;
}

static int
fun_count_item (env_t * env, int args_nr, val_t * result, val_t * args)
{
    character_t *chr = (ETY (0) == BL_PC) ? ARGPC (0) : NULL;
    int  stackable;
    struct item item;

    GET_ARG_ITEM (1, item, stackable);

    if (!chr)
        return 1;

    RESULTINT = pc_count_all_items (chr, item.nameid);
    return 0;
}

static int
fun_is_equipped (env_t * env, int args_nr, val_t * result, val_t * args)
{
    character_t *chr = (ETY (0) == BL_PC) ? ARGPC (0) : NULL;
    int  stackable;
    struct item item;
    int  i;
    int  retval = 0;

    GET_ARG_ITEM (1, item, stackable);

    if (!chr)
        return 1;

    for (i = 0; i < 11; i++)
        if (chr->equip_index[i] >= 0
            && chr->status.inventory[chr->equip_index[i]].nameid ==
            item.nameid)
        {
            retval = i + 1;
            break;
        }

    RESULTINT = retval;
    return 0;
}

static int
fun_is_married (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = (ETY (0) == BL_PC && ARGPC (0)->status.partner_id);
    return 0;
}

static int
fun_is_dead (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = (ETY (0) == BL_PC && pc_isdead (ARGPC (0)));
    return 0;
}

static int fun_is_pc (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = (ETY (0) == BL_PC);
    return 0;
}

static int
fun_partner (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (ETY (0) == BL_PC && ARGPC (0)->status.partner_id)
    {
        RESULTENTITY =
            (entity_t *)
            map_nick2sd (map_charid2nick (ARGPC (0)->status.partner_id));
        return 0;
    }
    else
        return 1;
}

static int
fun_awayfrom (env_t * env, int args_nr, val_t * result, val_t * args)
{
    location_t *loc = &ARGLOCATION (0);
    int  dx = heading_x[ARGDIR (1)];
    int  dy = heading_y[ARGDIR (1)];
    int  distance = ARGINT (2);
    while (distance-- && !map_is_solid (loc->m, loc->x + dx, loc->y + dy))
    {
        loc->x += dx;
        loc->y += dy;
    }

    RESULTLOCATION = *loc;
    return 0;
}

static int fun_failed (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = TY (0) == TY_FAIL;
    return 0;
}

static int fun_npc (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTENTITY = (entity_t *) npc_name2id (ARGSTR (0));
    return RESULTENTITY == NULL;
}

static int fun_pc (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTENTITY = (entity_t *) map_nick2sd (ARGSTR (0));
    return RESULTENTITY == NULL;
}

static int
fun_distance (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (ARGLOCATION (0).m != ARGLOCATION (1).m)
        RESULTINT = INT_MAX;
    else
        RESULTINT = MAX (abs (ARGLOCATION (0).x - ARGLOCATION (1).x),
                         abs (ARGLOCATION (0).y - ARGLOCATION (1).y));
    return 0;
}

static int
fun_rdistance (env_t * env, int args_nr, val_t * result, val_t * args)
{
    if (ARGLOCATION (0).m != ARGLOCATION (1).m)
        RESULTINT = INT_MAX;
    else
    {
        int  dx = ARGLOCATION (0).x - ARGLOCATION (1).x;
        int  dy = ARGLOCATION (0).y - ARGLOCATION (1).y;
        RESULTINT = (int) (sqrt ((dx * dx) + (dy * dy)));
    }
    return 0;
}

static int fun_anchor (env_t * env, int args_nr, val_t * result, val_t * args)
{
    teleport_anchor_t *anchor = magic_find_anchor (ARGSTR (0));

    if (!anchor)
        return 1;

    magic_eval (env, result, anchor->location);

    make_area (result);
    if (result->ty != TY_AREA)
    {
        magic_clear_var (result);
        return 1;
    }

    return 0;
}

static int
fun_line_of_sight (env_t * env, int args_nr, val_t * result, val_t * args)
{
    entity_t e1, e2;

    COPY_LOCATION (e1, ARGLOCATION (0));
    COPY_LOCATION (e2, ARGLOCATION (1));

    RESULTINT = battle_check_range (&e1, &e2, 0);

    return 0;
}

void magic_random_location (location_t * dest, area_t * area)
{
    switch (area->ty)
    {
        case AREA_UNION:
        {
            int  rv = MRAND (area->size);
            if (rv < area->a.a_union[0]->size)
                magic_random_location (dest, area->a.a_union[0]);
            else
                magic_random_location (dest, area->a.a_union[1]);
            break;
        }

        case AREA_LOCATION:
        case AREA_RECT:
        case AREA_BAR:
        {
            int  m, x, y, w, h;
            magic_area_rect (&m, &x, &y, &w, &h, area);

            if (w <= 1)
                w = 1;

            if (h <= 1)
                h = 1;

            x += MRAND (w);
            y += MRAND (h);

            if (!map_is_solid (m, x, y))
            {
                int  start_x = x;
                int  start_y = y;
                int  i;
                int  initial_dir = mt_random () & 0x7;
                int  dir = initial_dir;

                /* try all directions, up to a distance to 10, for a free slot */
                do
                {
                    x = start_x;
                    y = start_y;

                    for (i = 0; i < 10 && map_is_solid (m, x, y); i++)
                    {
                        x += heading_x[dir];
                        y += heading_y[dir];
                    }

                    dir = (dir + 1) & 0x7;
                }
                while (map_is_solid (m, x, y) && dir != initial_dir);

            }
            /* We've tried our best.  If the map is still solid, the engine will automatically randomise the target location if we try to warp. */

            dest->m = m;
            dest->x = x;
            dest->y = y;
            break;
        }

        default:
            fprintf (stderr, "Unknown area type %d\n", area->ty);
    }
}

static int
fun_pick_location (env_t * env, int args_nr, val_t * result, val_t * args)
{
    magic_random_location (&result->v.v_location, ARGAREA (0));
    return 0;
}

static int
fun_read_script_int (env_t * env, int args_nr, val_t * result, val_t * args)
{
    entity_t *subject_p = ARGENTITY (0);
    char *var_name = ARGSTR (1);

    if (subject_p->type != BL_PC)
        return 1;

    RESULTINT = pc_readglobalreg ((character_t *) subject_p, var_name);
    return 0;
}

static int fun_rbox (env_t * env, int args_nr, val_t * result, val_t * args)
{
    location_t loc = ARGLOCATION (0);
    int  radius = ARGINT (1);

    RESULTAREA = area_new (AREA_RECT);
    RESULTAREA->a.a_rect.loc.m = loc.m;
    RESULTAREA->a.a_rect.loc.x = loc.x - radius;
    RESULTAREA->a.a_rect.loc.y = loc.y - radius;
    RESULTAREA->a.a_rect.width = radius * 2 + 1;
    RESULTAREA->a.a_rect.height = radius * 2 + 1;

    return 0;
}

static int
fun_running_status_update (env_t * env, int args_nr, val_t * result,
                           val_t * args)
{
    if (ETY (0) != BL_PC && ETY (0) != BL_MOB)
        return 1;

    RESULTINT = battle_get_sc_data (ARGENTITY (0))[ARGINT (1)].timer != -1;
    return 0;
}

static int
fun_status_option (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT =
        ((((struct map_session_data *) ARGENTITY (0))->
          status.option & ARGINT (0)) != 0);
    return 0;
}

static int
fun_element (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = battle_get_element (ARGENTITY (0)) % 10;
    return 0;
}

static int
fun_element_level (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = battle_get_element (ARGENTITY (0)) / 10;
    return 0;
}

static int fun_index (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = ARGSPELL (0)->index;
    return 0;
}

static int
fun_is_exterior (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = map[ARGLOCATION (0).m].name[4] == '1';
    return 0;
}

static int
fun_contains_string (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = NULL != strstr (ARGSTR (0), ARGSTR (1));
    return 0;
}

static int fun_strstr (env_t * env, int args_nr, val_t * result, val_t * args)
{
    char *offset = strstr (ARGSTR (0), ARGSTR (1));
    RESULTINT = offset - ARGSTR (0);
    return offset == NULL;
}

static int fun_strlen (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = strlen (ARGSTR (0));
    return 0;
}

static int fun_substr (env_t * env, int args_nr, val_t * result, val_t * args)
{
    const char *src = ARGSTR (0);
    const int slen = strlen (src);
    int  offset = ARGINT (1);
    int  len = ARGINT (2);

    if (len < 0)
        len = 0;
    if (offset < 0)
        offset = 0;

    if (offset > slen)
        offset = slen;

    if (offset + len > slen)
        len = slen - offset;

    RESULTSTR = (char *) calloc (1, 1 + len);
    memcpy (RESULTSTR, src + offset, len);

    return 0;
}

static int fun_sqrt (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = (int) sqrt (ARGINT (0));
    return 0;
}

static int
fun_map_level (env_t * env, int args_nr, val_t * result, val_t * args)
{
    RESULTINT = map[ARGLOCATION (0).m].name[4] - '0';
    return 0;
}

static int fun_map_nr (env_t * env, int args_nr, val_t * result, val_t * args)
{
    const char *mapname = map[ARGLOCATION (0).m].name;

    RESULTINT = ((mapname[0] - '0') * 100)
        + ((mapname[1] - '0') * 10) + ((mapname[2] - '0'));
    return 0;
}

static int
fun_dir_towards (env_t * env, int args_nr, val_t * result, val_t * args)
{
    int  dx;
    int  dy;

    if (ARGLOCATION (0).m != ARGLOCATION (1).m)
        return 1;

    dx = ARGLOCATION (1).x - ARGLOCATION (0).x;
    dy = ARGLOCATION (1).y - ARGLOCATION (0).y;

    if (ARGINT (1))
    {
        /* 8-direction mode */
        if (abs (dx) > abs (dy) * 2)
        {                       /* east or west */
            if (dx < 0)
                RESULTINT = 2 /* west */ ;
            else
                RESULTINT = 6 /* east */ ;
        }
        else if (abs (dy) > abs (dx) * 2)
        {                       /* north or south */
            if (dy > 0)
                RESULTINT = 0 /* south */ ;
            else
                RESULTINT = 4 /* north */ ;
        }
        else if (dx < 0)
        {                       /* north-west or south-west */
            if (dy < 0)
                RESULTINT = 3 /* north-west */ ;
            else
                RESULTINT = 1 /* south-west */ ;
        }
        else
        {                       /* north-east or south-east */
            if (dy < 0)
                RESULTINT = 5 /* north-east */ ;
            else
                RESULTINT = 7 /* south-east */ ;
        }
    }
    else
    {
        /* 4-direction mode */
        if (abs (dx) > abs (dy))
        {                       /* east or west */
            if (dx < 0)
                RESULTINT = 2 /* west */ ;
            else
                RESULTINT = 6 /* east */ ;
        }
        else
        {                       /* north or south */
            if (dy > 0)
                RESULTINT = 0 /* south */ ;
            else
                RESULTINT = 4 /* north */ ;
        }
    }

    return 0;
}

static int
fun_extract_healer_xp (env_t * env, int args_nr, val_t * result, val_t * args)
{
    character_t *sd = (ETY (0) == BL_PC) ? ARGPC (0) : NULL;

    if (!sd)
        RESULTINT = 0;
    else
        RESULTINT = pc_extract_healer_exp (sd, ARGINT (1));
    return 0;
}

#define BATTLE_RECORD2(sname, name) { sname, "e", 'i', fun_get_##name }
#define BATTLE_RECORD(name) BATTLE_RECORD2(#name, name)
static fun_t functions[] = {
    {"+", "..", '.', fun_add},
    {"-", "ii", 'i', fun_sub},
    {"*", "ii", 'i', fun_mul},
    {"/", "ii", 'i', fun_div},
    {"%", "ii", 'i', fun_mod},
    {"||", "ii", 'i', fun_or},
    {"&&", "ii", 'i', fun_and},
    {">", "..", 'i', fun_gt},
    {">=", "..", 'i', fun_gte},
    {"=", "..", 'i', fun_eq},
    {"|", "..", 'i', fun_bitor},
    {"&", "ii", 'i', fun_bitand},
    {"^", "ii", 'i', fun_bitxor},
    {"<<", "ii", 'i', fun_bitshl},
    {">>", "ii", 'i', fun_bitshr},
    {"not", "i", 'i', fun_not},
    {"neg", "i", 'i', fun_neg},
    {"max", "ii", 'i', fun_max},
    {"min", "ii", 'i', fun_min},
    {"is_in", "la", 'i', fun_is_in},
    {"if_then_else", "i__", '_', fun_if_then_else},
    {"skill", "ei", 'i', fun_skill},
    BATTLE_RECORD (str),
    BATTLE_RECORD (agi),
    BATTLE_RECORD (vit),
    BATTLE_RECORD (dex),
    BATTLE_RECORD (luk),
    BATTLE_RECORD (int),
    BATTLE_RECORD2 ("level", lv),
    BATTLE_RECORD (mdef),
    BATTLE_RECORD (def),
    BATTLE_RECORD (hp),
    BATTLE_RECORD (max_hp),
    BATTLE_RECORD (sp),
    BATTLE_RECORD (max_sp),
    {"dir", "e", 'd', fun_get_dir},
    {"name_of", ".", 's', fun_name_of},
    {"mob_id", "e", 'i', fun_mob_id},
    {"location", "e", 'l', fun_location},
    {"random", "i", 'i', fun_random},
    {"random_dir", "i", 'd', fun_random_dir},
    {"hash_entity", "e", 'i', fun_hash_entity},
    {"is_married", "e", 'i', fun_is_married},
    {"partner", "e", 'e', fun_partner},
    {"awayfrom", "ldi", 'l', fun_awayfrom},
    {"failed", "_", 'i', fun_failed},
    {"pc", "s", 'e', fun_pc},
    {"npc", "s", 'e', fun_npc},
    {"distance", "ll", 'i', fun_distance},
    {"rdistance", "ll", 'i', fun_rdistance},
    {"anchor", "s", 'a', fun_anchor},
    {"random_location", "a", 'l', fun_pick_location},
    {"script_int", "es", 'i', fun_read_script_int},
    {"rbox", "li", 'a', fun_rbox},
    {"count_item", "e.", 'i', fun_count_item},
    {"line_of_sight", "ll", 'i', fun_line_of_sight},
    {"running_status_update", "ei", 'i', fun_running_status_update},
    {"status_option", "ei", 'i', fun_status_option},
    {"element", "e", 'i', fun_element},
    {"element_level", "e", 'i', fun_element_level},
    {"has_shroud", "e", 'i', fun_has_shroud},
    {"is_equipped", "e.", 'i', fun_is_equipped},
    {"spell_index", "S", 'i', fun_index},
    {"is_exterior", "l", 'i', fun_is_exterior},
    {"contains_string", "ss", 'i', fun_contains_string},
    {"strstr", "ss", 'i', fun_strstr},
    {"strlen", "s", 'i', fun_strlen},
    {"substr", "sii", 's', fun_substr},
    {"sqrt", "i", 'i', fun_sqrt},
    {"map_level", "l", 'i', fun_map_level},
    {"map_nr", "l", 'i', fun_map_nr},
    {"dir_towards", "lli", 'd', fun_dir_towards},
    {"is_dead", "e", 'i', fun_is_dead},
    {"is_pc", "e", 'i', fun_is_pc},
    {"extract_healer_experience", "ei", 'i', fun_extract_healer_xp},
    {NULL, NULL, '.', NULL}
};

static int functions_are_sorted = 0;

int compare_fun (const void *lhs, const void *rhs)
{
    return strcmp (((fun_t *) lhs)->name, ((fun_t *) rhs)->name);
}

fun_t *magic_get_fun (char *name, int *index)
{
    static int functions_nr;
    fun_t *result;
    fun_t key;

    if (!functions_are_sorted)
    {
        fun_t *it = functions;

        while (it->name)
            ++it;
        functions_nr = it - functions;

        qsort (functions, functions_nr, sizeof (fun_t), compare_fun);
        functions_are_sorted = 1;
    }

    key.name = name;
    result = (fun_t *) bsearch (&key, functions, functions_nr, sizeof (fun_t),
                                compare_fun);

    if (result && index)
        *index = result - functions;

    return result;
}

static int                      // 1 on failure
eval_location (env_t * env, location_t * dest, e_location_t * expr)
{
    val_t m, x, y;
    magic_eval (env, &m, expr->m);
    magic_eval (env, &x, expr->x);
    magic_eval (env, &y, expr->y);

    if (CHECK_TYPE (&m, TY_STRING)
        && CHECK_TYPE (&x, TY_INT) && CHECK_TYPE (&y, TY_INT))
    {
        int  map_id = map_mapname2mapid (m.v.v_string);
        magic_clear_var (&m);
        if (map_id < 0)
            return 1;
        dest->m = map_id;
        dest->x = x.v.v_int;
        dest->y = y.v.v_int;
        return 0;
    }
    else
    {
        magic_clear_var (&m);
        magic_clear_var (&x);
        magic_clear_var (&y);
        return 1;
    }
}

static area_t *eval_area (env_t * env, e_area_t * expr)
{
    area_t *area = malloc (sizeof (area_t));
    area->ty = expr->ty;

    switch (expr->ty)
    {
        case AREA_LOCATION:
            area->size = 1;
            if (eval_location (env, &area->a.a_loc, &expr->a.a_loc))
            {
                free (area);
                return NULL;
            }
            else
                return area;

        case AREA_UNION:
        {
            int  i, fail = 0;
            for (i = 0; i < 2; i++)
            {
                area->a.a_union[i] = eval_area (env, expr->a.a_union[i]);
                if (!area->a.a_union[i])
                    fail = 1;
            }

            if (fail)
            {
                for (i = 0; i < 2; i++)
                {
                    if (area->a.a_union[i])
                        free_area (area->a.a_union[i]);
                }
                free (area);
                return NULL;
            }
            area->size = area->a.a_union[0]->size + area->a.a_union[1]->size;
            return area;
        }

        case AREA_RECT:
        {
            val_t width, height;
            magic_eval (env, &width, expr->a.a_rect.width);
            magic_eval (env, &height, expr->a.a_rect.height);

            area->a.a_rect.width = width.v.v_int;
            area->a.a_rect.height = height.v.v_int;

            if (CHECK_TYPE (&width, TY_INT)
                && CHECK_TYPE (&height, TY_INT)
                && !eval_location (env, &(area->a.a_rect.loc),
                                   &expr->a.a_rect.loc))
            {
                area->size = area->a.a_rect.width * area->a.a_rect.height;
                magic_clear_var (&width);
                magic_clear_var (&height);
                return area;
            }
            else
            {
                free (area);
                magic_clear_var (&width);
                magic_clear_var (&height);
                return NULL;
            }
        }

        case AREA_BAR:
        {
            val_t width, depth, dir;
            magic_eval (env, &width, expr->a.a_bar.width);
            magic_eval (env, &depth, expr->a.a_bar.depth);
            magic_eval (env, &dir, expr->a.a_bar.dir);

            area->a.a_bar.width = width.v.v_int;
            area->a.a_bar.depth = depth.v.v_int;
            area->a.a_bar.dir = dir.v.v_int;

            if (CHECK_TYPE (&width, TY_INT)
                && CHECK_TYPE (&depth, TY_INT)
                && CHECK_TYPE (&dir, TY_DIR)
                && !eval_location (env, &area->a.a_bar.loc,
                                   &expr->a.a_bar.loc))
            {
                area->size =
                    (area->a.a_bar.width * 2 + 1) * area->a.a_bar.depth;
                magic_clear_var (&width);
                magic_clear_var (&depth);
                magic_clear_var (&dir);
                return area;
            }
            else
            {
                free (area);
                magic_clear_var (&width);
                magic_clear_var (&depth);
                magic_clear_var (&dir);
                return NULL;
            }
        }

        default:
            fprintf (stderr, "INTERNAL ERROR: Unknown area type %d\n",
                     area->ty);
            free (area);
            return NULL;
    }
}

static int type_key (char ty_key)
{
    switch (ty_key)
    {
        case 'i':
            return TY_INT;
        case 'd':
            return TY_DIR;
        case 's':
            return TY_STRING;
        case 'e':
            return TY_ENTITY;
        case 'l':
            return TY_LOCATION;
        case 'a':
            return TY_AREA;
        case 'S':
            return TY_SPELL;
        case 'I':
            return TY_INVOCATION;
        default:
            return -1;
    }
}

int
magic_signature_check (char *opname, char *funname, char *signature,
                       int args_nr, val_t * args, int line, int column)
{
    int  i;
    for (i = 0; i < args_nr; i++)
    {
        val_t *arg = &args[i];
        char ty_key = signature[i];
        int  ty = arg->ty;
        int  desired_ty = type_key (ty_key);

        if (ty == TY_ENTITY)
        {
            /* Dereference entities in preparation for calling function */
            arg->v.v_entity = map_id2bl (arg->v.v_int);
            if (!arg->v.v_entity)
                ty = arg->ty = TY_FAIL;
        }
        else if (ty == TY_INVOCATION)
        {
            arg->v.v_invocation = (invocation_t *) map_id2bl (arg->v.v_int);
            if (!arg->v.v_entity)
                ty = arg->ty = TY_FAIL;
        }

        if (!ty_key)
        {
            fprintf (stderr,
                     "[magic-eval]:  L%d:%d: Too many arguments (%d) to %s `%s'\n",
                     line, column, args_nr, opname, funname);
            return 1;
        }

        if (ty == TY_FAIL && ty_key != '_')
            return 1;           /* Fail `in a sane way':  This is a perfectly permissible error */

        if (ty == desired_ty || desired_ty < 0 /* `dontcare' */ )
            continue;

        if (ty == TY_UNDEF)
        {
            fprintf (stderr,
                     "[magic-eval]:  L%d:%d: Argument #%d to %s `%s' undefined\n",
                     line, column, i + 1, opname, funname);
            return 1;
        }

        /* If we are here, we have a type mismatch but no failure _yet_.  Try to coerce. */
        switch (desired_ty)
        {
            case TY_INT:
                intify (arg);
                break;          /* 100% success rate */
            case TY_STRING:
                stringify (arg, 1);
                break;          /* 100% success rate */
            case TY_AREA:
                make_area (arg);
                break;          /* Only works for locations */
            case TY_LOCATION:
                make_location (arg);
                break;          /* Only works for some areas */
            case TY_SPELL:
                make_spell (arg);
                break;          /* Only works for still-active invocatoins */
            default:
                break;          /* We'll fail right below */
        }

        ty = arg->ty;
        if (ty != desired_ty)
        {                       /* Coercion failed? */
            if (ty != TY_FAIL)
                fprintf (stderr,
                         "[magic-eval]:  L%d:%d: Argument #%d to %s `%s' of incorrect type (%d)\n",
                         line, column, i + 1, opname, funname, ty);
            return 1;
        }
    }

    return 0;
}

void magic_eval (env_t * env, val_t * dest, expr_t * expr)
{
    switch (expr->ty)
    {
        case EXPR_VAL:
            magic_copy_var (dest, &expr->e.e_val);
            break;

        case EXPR_LOCATION:
            if (eval_location (env, &dest->v.v_location, &expr->e.e_location))
                dest->ty = TY_FAIL;
            else
                dest->ty = TY_LOCATION;
            break;

        case EXPR_AREA:
            if ((dest->v.v_area = eval_area (env, &expr->e.e_area)))
                dest->ty = TY_AREA;
            else
                dest->ty = TY_FAIL;
            break;

        case EXPR_FUNAPP:
        {
            val_t arguments[MAX_ARGS];
            int  args_nr = expr->e.e_funapp.args_nr;
            int  i;
            fun_t *f = functions + expr->e.e_funapp.id;

            for (i = 0; i < args_nr; ++i)
                magic_eval (env, &arguments[i], expr->e.e_funapp.args[i]);
            if (magic_signature_check
                ("function", f->name, f->signature, args_nr, arguments,
                 expr->e.e_funapp.line_nr, expr->e.e_funapp.column)
                || f->fun (env, args_nr, dest, arguments))
                dest->ty = TY_FAIL;
            else
            {
                int  dest_ty = type_key (f->ret_ty);
                if (dest_ty != -1)
                    dest->ty = dest_ty;

                /* translate entity back into persistent int */
                if (dest->ty == TY_ENTITY)
                {
                    if (dest->v.v_entity)
                        dest->v.v_int = dest->v.v_entity->id;
                    else
                        dest->ty = TY_FAIL;
                }
            }

            for (i = 0; i < args_nr; ++i)
                magic_clear_var (&arguments[i]);
            break;
        }

        case EXPR_ID:
        {
            val_t v = VAR (expr->e.e_id);
            magic_copy_var (dest, &v);
            break;
        }

        case EXPR_SPELLFIELD:
        {
            val_t v;
            int  id = expr->e.e_field.id;
            magic_eval (env, &v, expr->e.e_field.expr);

            if (v.ty == TY_INVOCATION)
            {
                invocation_t *t = (invocation_t *) map_id2bl (v.v.v_int);

                if (!t)
                    dest->ty = TY_UNDEF;
                else
                {
                    env_t *env = t->env;
                    val_t v = VAR (id);
                    magic_copy_var (dest, &v);
                }
            }
            else
            {
                fprintf (stderr,
                         "[magic] Attempt to access field %s on non-spell\n",
                         env->base_env->var_name[id]);
                dest->ty = TY_FAIL;
            }
            break;
        }

        default:
            fprintf (stderr,
                     "[magic] INTERNAL ERROR: Unknown expression type %d\n",
                     expr->ty);
            break;
    }
}

int magic_eval_int (env_t * env, expr_t * expr)
{
    val_t result;
    magic_eval (env, &result, expr);

    if (result.ty == TY_FAIL || result.ty == TY_UNDEF)
        return 0;

    intify (&result);

    return result.v.v_int;
}

char *magic_eval_str (env_t * env, expr_t * expr)
{
    val_t result;
    magic_eval (env, &result, expr);

    if (result.ty == TY_FAIL || result.ty == TY_UNDEF)
        return strdup ("?");

    stringify (&result, 0);

    return result.v.v_string;
}

expr_t *magic_new_expr (int ty)
{
    expr_t *expr = (expr_t *) malloc (sizeof (expr_t));
    expr->ty = ty;
    return expr;
}

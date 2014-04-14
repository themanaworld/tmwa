#include "path.hpp"
//    path.cpp - Pathfinding system.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013 Stefan Dombrowski
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

#include <cassert>

#include "../compat/nullpo.hpp"

#include "../generic/random.hpp"

#include "../io/cxxstdio.hpp"

#include "battle.hpp"

#include "../poison.hpp"

//#define PATH_STANDALONETEST

constexpr int MAX_HEAP = 150;
struct tmp_path
{
    short x, y, dist, before, cost;
    DIR dir;
    char flag;
};

static
int calc_index(int x, int y)
{
    return (x + y * MAX_WALKPATH) % (MAX_WALKPATH * MAX_WALKPATH);
}

/*==========================================
 * 経路探索補助heap push
 *------------------------------------------
 */
static
void push_heap_path(int *heap, struct tmp_path *tp, int index)
{
    int i, h;

    if (heap == NULL || tp == NULL)
    {
        PRINTF("push_heap_path nullpo\n"_fmt);
        return;
    }

    heap[0]++;

    for (h = heap[0] - 1, i = (h - 1) / 2;
         h > 0 && tp[index].cost < tp[heap[i + 1]].cost; i = (h - 1) / 2)
        heap[h + 1] = heap[i + 1], h = i;
    heap[h + 1] = index;
}

/*==========================================
 * 経路探索補助heap update
 * costが減ったので根の方へ移動
 *------------------------------------------
 */
static
void update_heap_path(int *heap, struct tmp_path *tp, int index)
{
    int i, h;

    nullpo_retv(heap);
    nullpo_retv(tp);

    for (h = 0; h < heap[0]; h++)
        if (heap[h + 1] == index)
            break;
    if (h == heap[0])
    {
        FPRINTF(stderr, "update_heap_path bug\n"_fmt);
        exit(1);
    }
    for (i = (h - 1) / 2;
         h > 0 && tp[index].cost < tp[heap[i + 1]].cost; i = (h - 1) / 2)
        heap[h + 1] = heap[i + 1], h = i;
    heap[h + 1] = index;
}

/*==========================================
 * 経路探索補助heap pop
 *------------------------------------------
 */
static
int pop_heap_path(int *heap, struct tmp_path *tp)
{
    int i, h, k;
    int ret, last;

    nullpo_retr(-1, heap);
    nullpo_retr(-1, tp);

    if (heap[0] <= 0)
        return -1;
    ret = heap[1];
    last = heap[heap[0]];
    heap[0]--;

    for (h = 0, k = 2; k < heap[0]; k = k * 2 + 2)
    {
        if (tp[heap[k + 1]].cost > tp[heap[k]].cost)
            k--;
        heap[h + 1] = heap[k + 1], h = k;
    }
    if (k == heap[0])
        heap[h + 1] = heap[k], h = k - 1;

    for (i = (h - 1) / 2;
         h > 0 && tp[heap[i + 1]].cost > tp[last].cost; i = (h - 1) / 2)
        heap[h + 1] = heap[i + 1], h = i;
    heap[h + 1] = last;

    return ret;
}

/*==========================================
 * 現在の点のcost計算
 *------------------------------------------
 */
static
int calc_cost(struct tmp_path *p, int x1, int y1)
{
    int xd, yd;

    nullpo_ret(p);

    xd = x1 - p->x;
    if (xd < 0)
        xd = -xd;
    yd = y1 - p->y;
    if (yd < 0)
        yd = -yd;
    return (xd + yd) * 10 + p->dist;
}

/*==========================================
 * 必要ならpathを追加/修正する
 *------------------------------------------
 */
static
int add_path(int *heap, struct tmp_path *tp, int x, int y, int dist,
        DIR dir, int before, int x1, int y1)
{
    int i;

    nullpo_ret(heap);
    nullpo_ret(tp);

    i = calc_index(x, y);

    if (tp[i].x == x && tp[i].y == y)
    {
        if (tp[i].dist > dist)
        {
            tp[i].dist = dist;
            tp[i].dir = dir;
            tp[i].before = before;
            tp[i].cost = calc_cost(&tp[i], x1, y1);
            if (tp[i].flag)
                push_heap_path(heap, tp, i);
            else
                update_heap_path(heap, tp, i);
            tp[i].flag = 0;
        }
        return 0;
    }

    if (tp[i].x || tp[i].y)
        return 1;

    tp[i].x = x;
    tp[i].y = y;
    tp[i].dist = dist;
    tp[i].dir = dir;
    tp[i].before = before;
    tp[i].cost = calc_cost(&tp[i], x1, y1);
    tp[i].flag = 0;
    push_heap_path(heap, tp, i);

    return 0;
}

/*==========================================
 * (x,y)が移動不可能地帯かどうか
 * flag 0x10000 遠距離攻撃判定
 *------------------------------------------
 */
static
bool can_place(struct map_local *m, int x, int y)
{
    nullpo_ret(m);

    return !bool(read_gatp(m, x, y) & MapCell::UNWALKABLE);
}

/*==========================================
 * (x0,y0)から(x1,y1)へ1歩で移動可能か計算
 *------------------------------------------
 */
static
int can_move(struct map_local *m, int x0, int y0, int x1, int y1)
{
    nullpo_ret(m);

    if (x0 - x1 < -1 || x0 - x1 > 1 || y0 - y1 < -1 || y0 - y1 > 1)
        return 0;
    if (x1 < 0 || y1 < 0 || x1 >= m->xs || y1 >= m->ys)
        return 0;
    if (!can_place(m, x0, y0))
        return 0;
    if (!can_place(m, x1, y1))
        return 0;
    if (x0 == x1 || y0 == y1)
        return 1;
    if (!can_place(m, x0, y1) || !can_place(m, x1, y0))
        return 0;
    return 1;
}

/*==========================================
 * path探索 (x0,y0)->(x1,y1)
 *------------------------------------------
 */
int path_search(struct walkpath_data *wpd, map_local *m, int x0, int y0, int x1, int y1, int flag)
{
    int heap[MAX_HEAP + 1];
    int i, rp, x, y;
    int dx, dy;

    nullpo_ret(wpd);

    assert (m->gat);
    map_local *md = m;
    if (x1 < 0 || x1 >= md->xs || y1 < 0 || y1 >= md->ys
        || bool(read_gatp(md, x1, y1) & MapCell::UNWALKABLE))
        return -1;

    // easy
    dx = (x1 - x0 < 0) ? -1 : 1;
    dy = (y1 - y0 < 0) ? -1 : 1;
    for (x = x0, y = y0, i = 0; x != x1 || y != y1;)
    {
        if (i >= sizeof(wpd->path))
            return -1;
        if (x != x1 && y != y1)
        {
            if (!can_move(md, x, y, x + dx, y + dy))
                break;
            x += dx;
            y += dy;
            wpd->path[i++] = (dx < 0)
                ? ((dy > 0) ? DIR::SW : DIR::NW)
                : ((dy < 0) ? DIR::NE : DIR::SE);
        }
        else if (x != x1)
        {
            if (!can_move(md, x, y, x + dx, y))
                break;
            x += dx;
            wpd->path[i++] = (dx < 0) ? DIR::W : DIR::E;
        }
        else
        {                       // y!=y1
            if (!can_move(md, x, y, x, y + dy))
                break;
            y += dy;
            wpd->path[i++] = (dy > 0) ? DIR::S : DIR::N;
        }
        if (x == x1 && y == y1)
        {
            wpd->path_len = i;
            wpd->path_pos = 0;
            wpd->path_half = 0;
            return 0;
        }
    }
    if (flag & 1)
        return -1;

    struct tmp_path tp[MAX_WALKPATH * MAX_WALKPATH] {};

    i = calc_index(x0, y0);
    tp[i].x = x0;
    tp[i].y = y0;
    tp[i].dist = 0;
    tp[i].dir = DIR::S;
    tp[i].before = 0;
    tp[i].cost = calc_cost(&tp[i], x1, y1);
    tp[i].flag = 0;
    heap[0] = 0;
    push_heap_path(heap, tp, calc_index(x0, y0));
    while (1)
    {
        int e = 0;

        if (heap[0] == 0)
            return -1;
        rp = pop_heap_path(heap, tp);
        x = tp[rp].x;
        y = tp[rp].y;
        if (x == x1 && y == y1)
        {
            int len, j;

            for (len = 0, i = rp; len < 100 && i != calc_index(x0, y0);
                 i = tp[i].before, len++);
            if (len == 100 || len >= sizeof(wpd->path))
                return -1;
            wpd->path_len = len;
            wpd->path_pos = 0;
            wpd->path_half = 0;
            for (i = rp, j = len - 1; j >= 0; i = tp[i].before, j--)
                wpd->path[j] = tp[i].dir;

            return 0;
        }
        if (can_move(md, x, y, x + 1, y - 1))
            e += add_path(heap, tp, x + 1, y - 1, tp[rp].dist + 14, DIR::NE, rp, x1, y1);
        if (can_move(md, x, y, x + 1, y))
            e += add_path(heap, tp, x + 1, y, tp[rp].dist + 10, DIR::E, rp, x1, y1);
        if (can_move(md, x, y, x + 1, y + 1))
            e += add_path(heap, tp, x + 1, y + 1, tp[rp].dist + 14, DIR::SE, rp, x1, y1);
        if (can_move(md, x, y, x, y + 1))
            e += add_path(heap, tp, x, y + 1, tp[rp].dist + 10, DIR::S, rp, x1, y1);
        if (can_move(md, x, y, x - 1, y + 1))
            e += add_path(heap, tp, x - 1, y + 1, tp[rp].dist + 14, DIR::SW, rp, x1, y1);
        if (can_move(md, x, y, x - 1, y))
            e += add_path(heap, tp, x - 1, y, tp[rp].dist + 10, DIR::W, rp, x1, y1);
        if (can_move(md, x, y, x - 1, y - 1))
            e += add_path(heap, tp, x - 1, y - 1, tp[rp].dist + 14, DIR::NW, rp, x1, y1);
        if (can_move(md, x, y, x, y - 1))
            e += add_path(heap, tp, x, y - 1, tp[rp].dist + 10, DIR::N, rp, x1, y1);
        tp[rp].flag = 1;
        if (e || heap[0] >= MAX_HEAP - 5)
            return -1;
    }
}

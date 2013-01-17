#include "timer.hpp"

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "cxxstdio.hpp"
#include "utils.hpp"

#include "../poison.hpp"

static
struct TimerData *timer_data;
static
uint32_t timer_data_max, timer_data_num;
static
timer_id *free_timer_list;
static
uint32_t free_timer_list_max, free_timer_list_pos;

/// Okay, I think I understand this structure now:
/// the timer heap is a magic queue that allows inserting timers and then popping them in order
/// designed to copy only log2(N) entries instead of N
// timer_heap[0] is the size (greatest index into the heap)
// timer_heap[1] is the first actual element
// timer_heap_max increases 256 at a time and never decreases
static
uint32_t timer_heap_max = 0;
/// FIXME: refactor the code to put the size in a separate variable
//nontrivial because indices get multiplied
static
timer_id *timer_heap = NULL;


static
uint32_t gettick_cache;
static
uint8_t gettick_count = 0;

uint32_t gettick_nocache(void)
{
    struct timeval tval;
    // BUG: This will cause strange behavior if the system clock is changed!
    // it should be reimplemented in terms of clock_gettime(CLOCK_MONOTONIC, )
    gettimeofday(&tval, NULL);
    gettick_count = 255;
    return gettick_cache = tval.tv_sec * 1000 + tval.tv_usec / 1000;
}

uint32_t gettick(void)
{
    if (gettick_count--)
        return gettick_cache;
    return gettick_nocache();
}

static
void push_timer_heap(timer_id index)
{
    if (timer_heap == NULL || timer_heap[0] + 1 >= timer_heap_max)
    {
        timer_heap_max += 256;
        RECREATE(timer_heap, timer_id, timer_heap_max);
        memset(timer_heap + (timer_heap_max - 256), 0, sizeof(timer_id) * 256);
    }
// timer_heap[0] is the greatest index into the heap, which increases
    timer_heap[0]++;

    timer_id h = timer_heap[0]-1, i = (h - 1) / 2;
    while (h)
    {
        // avoid wraparound problems, it really means this:
        //   timer_data[index].tick >= timer_data[timer_heap[i+1]].tick
        if ( DIFF_TICK(timer_data[index].tick, timer_data[timer_heap[i+1]].tick) >= 0)
            break;
        timer_heap[h + 1] = timer_heap[i + 1];
        h = i;
        i = (h - 1) / 2;
    }
    timer_heap[h + 1] = index;
}

static
timer_id top_timer_heap(void)
{
    if (!timer_heap || !timer_heap[0])
        return -1;
    return timer_heap[1];
}

static
timer_id pop_timer_heap(void)
{
    if (!timer_heap || !timer_heap[0])
        return -1;
    timer_id ret = timer_heap[1];
    timer_id last = timer_heap[timer_heap[0]];
    timer_heap[0]--;

    uint32_t h, k;
    for (h = 0, k = 2; k < timer_heap[0]; k = k * 2 + 2)
    {
        if (DIFF_TICK(timer_data[timer_heap[k + 1]].tick, timer_data[timer_heap[k]].tick) > 0)
            k--;
        timer_heap[h + 1] = timer_heap[k + 1], h = k;
    }
    if (k == timer_heap[0])
        timer_heap[h + 1] = timer_heap[k], h = k - 1;

    uint32_t i = (h - 1) / 2;
    while (h)
    {
        if (DIFF_TICK(timer_data[timer_heap[i + 1]].tick, timer_data[last].tick) <= 0)
            break;
        timer_heap[h + 1] = timer_heap[i + 1];
        h = i;
        i = (h - 1) / 2;
    }
    timer_heap[h + 1] = last;

    return ret;
}

timer_id add_timer(tick_t tick, timer_func func, custom_id_t id, custom_data_t data)
{
    timer_id i;

    if (free_timer_list_pos)
    {
        // Retrieve a freed timer id instead of a new one
        // I think it should be possible to avoid the loop somehow
        do
        {
            i = free_timer_list[--free_timer_list_pos];
        }
        while (i >= timer_data_num && free_timer_list_pos > 0);
    }
    else
        i = timer_data_num;

    // I have no idea what this is doing
    if (i >= timer_data_num)
        for (i = timer_data_num; i < timer_data_max && timer_data[i].type; i++)
            ;
    if (i >= timer_data_num && i >= timer_data_max)
    {
        if (timer_data_max == 0)
        {
            timer_data_max = 256;
            CREATE(timer_data, struct TimerData, timer_data_max);
        }
        else
        {
            timer_data_max += 256;
            RECREATE(timer_data, struct TimerData, timer_data_max);
            memset(timer_data + (timer_data_max - 256), 0,
                    sizeof(struct TimerData) * 256);
        }
    }
    timer_data[i].tick = tick;
    timer_data[i].func = func;
    timer_data[i].id = id;
    timer_data[i].data = data;
    timer_data[i].type = TIMER_ONCE_AUTODEL;
    timer_data[i].interval = 1000;
    push_timer_heap(i);
    if (i >= timer_data_num)
        timer_data_num = i + 1;
    return i;
}

timer_id add_timer_interval(tick_t tick, timer_func func, custom_id_t id,
                             custom_data_t data, interval_t interval)
{
    timer_id tid = add_timer(tick, func, id, data);
    timer_data[tid].type = TIMER_INTERVAL;
    timer_data[tid].interval = interval;
    return tid;
}

void delete_timer(timer_id id, timer_func func)
{
    if (id == 0 || id >= timer_data_num)
    {
        FPRINTF(stderr, "delete_timer error : no such timer %d\n", id);
        abort();
    }
    if (timer_data[id].func != func)
    {
        FPRINTF(stderr, "Timer mismatch\n");
        abort();
    }
    // "to let them disappear" - is this just in case?
    timer_data[id].func = NULL;
    timer_data[id].type = TIMER_ONCE_AUTODEL;
    timer_data[id].tick -= 60 * 60 * 1000;
}

tick_t addtick_timer(timer_id tid, interval_t tick)
{
    return timer_data[tid].tick += tick;
}

struct TimerData *get_timer(timer_id tid)
{
    return &timer_data[tid];
}

interval_t do_timer(tick_t tick)
{
    timer_id i;
    /// Number of milliseconds until it calls this again
    // this says to wait 1 sec if all timers get popped
    interval_t nextmin = 1000;

    while ((i = top_timer_heap()) != (timer_id)-1)
    {
        // while the heap is not empty and
        if (DIFF_TICK(timer_data[i].tick, tick) > 0)
        {
            /// Return the time until the next timer needs to goes off
            nextmin = DIFF_TICK(timer_data[i].tick, tick);
            break;
        }
        pop_timer_heap();
        if (timer_data[i].func)
        {
            if (DIFF_TICK(timer_data[i].tick, tick) < -1000)
            {
                // If we are too far past the requested tick, call with the current tick instead to fix reregistering problems
                timer_data[i].func(i, tick, timer_data[i].id, timer_data[i].data);
            }
            else
            {
                timer_data[i].func(i, timer_data[i].tick, timer_data[i].id, timer_data[i].data);
            }
        }
        switch (timer_data[i].type)
        {
            case TIMER_ONCE_AUTODEL:
                timer_data[i].type = TIMER_NONE;
                if (free_timer_list_pos >= free_timer_list_max)
                {
                    free_timer_list_max += 256;
                    RECREATE(free_timer_list, uint32_t, free_timer_list_max);
                    memset(free_timer_list + (free_timer_list_max - 256),
                            0, 256 * sizeof(uint32_t));
                }
                free_timer_list[free_timer_list_pos++] = i;
                break;
            case TIMER_INTERVAL:
                if (DIFF_TICK(timer_data[i].tick, tick) < -1000)
                {
                    timer_data[i].tick = tick + timer_data[i].interval;
                }
                else
                {
                    timer_data[i].tick += timer_data[i].interval;
                }
                push_timer_heap(i);
                break;
        }
    }

    if (nextmin < 10)
        nextmin = 10;
    return nextmin;
}

#include "timer.hpp"

#include <sys/stat.h>
#include <sys/time.h>

#include <cassert>
#include <cstring>

#include <queue>

#include "cxxstdio.hpp"
#include "utils.hpp"

#include "../poison.hpp"

struct TimerData
{
    /// When it will be triggered
    tick_t tick;
    /// What will be done
    timer_func func;
    /// Repeat rate - 0 for oneshot
    interval_t interval;
};

struct TimerCompare
{
    /// implement "less than"
    bool operator() (TimerData *l, TimerData *r)
    {
        // C++ provides a max-heap, but we want
        // the smallest tick to be the head (a min-heap).
        return l->tick > r->tick;
    }
};

static
std::priority_queue<TimerData *, std::vector<TimerData *>, TimerCompare> timer_heap;


tick_t gettick_cache;

tick_t milli_clock::now(void) noexcept
{
    struct timeval tval;
    // BUG: This will cause strange behavior if the system clock is changed!
    // it should be reimplemented in terms of clock_gettime(CLOCK_MONOTONIC, )
    gettimeofday(&tval, NULL);
    return gettick_cache = tick_t(std::chrono::seconds(tval.tv_sec)
            + std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::microseconds(tval.tv_usec)));
}

static
void push_timer_heap(TimerData *td)
{
    timer_heap.push(td);
}

static
TimerData *top_timer_heap(void)
{
    if (timer_heap.empty())
        return nullptr;
    return timer_heap.top();
}

static
void pop_timer_heap(void)
{
    timer_heap.pop();
}

TimerData *add_timer(tick_t tick, timer_func func)
{
    return add_timer_interval(tick, std::move(func), interval_t::zero());
}

TimerData *add_timer_interval(tick_t tick, timer_func func, interval_t interval)
{
    assert (interval >= interval_t::zero());

    TimerData *td = new TimerData();
    td->tick = tick;
    td->func = std::move(func);
    td->interval = interval;
    push_timer_heap(td);
    return td;
}

static
void do_nothing(TimerData *, tick_t)
{
}

void delete_timer(TimerData *td)
{
    assert (td != nullptr);

    td->func = do_nothing;
    td->interval = interval_t::zero();
}

interval_t do_timer(tick_t tick)
{
    /// Number of milliseconds until it calls this again
    // this says to wait 1 sec if all timers get popped
    interval_t nextmin = std::chrono::seconds(1);

    while (TimerData *td = top_timer_heap())
    {
        // while the heap is not empty and
        if (td->tick > tick)
        {
            /// Return the time until the next timer needs to goes off
            nextmin = td->tick - tick;
            break;
        }
        pop_timer_heap();

        // If we are too far past the requested tick, call with the current tick instead to fix reregistering problems
        if (td->tick + std::chrono::seconds(1) < tick)
            td->func(td, tick);
        else
            td->func(td, td->tick);

        if (td->interval == interval_t::zero())
        {
            delete td;
            continue;
        }
        if (td->tick + std::chrono::seconds(1) < tick)
            td->tick = tick + td->interval;
        else
            td->tick += td->interval;
        push_timer_heap(td);
    }

    return std::max(nextmin, std::chrono::milliseconds(10));
}

tick_t file_modified(const char *name)
{
    struct stat buf;
    if (stat(name, &buf))
        return tick_t();
    return tick_t(std::chrono::seconds(buf.st_mtime));
}

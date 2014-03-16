#include "timer.hpp"

#include <sys/stat.h>
#include <sys/time.h>

#include <cassert>
#include <cstring>

#include <queue>

#include "../strings/zstring.hpp"

#include "../io/cxxstdio.hpp"

#include "utils.hpp"

#include "../poison.hpp"

struct TimerData
{
    /// This will be reset on call, to avoid problems.
    Timer *owner;

    /// When it will be triggered
    tick_t tick;
    /// What will be done
    timer_func func;
    /// Repeat rate - 0 for oneshot
    interval_t interval;

    TimerData(Timer *o, tick_t t, timer_func f, interval_t i)
    : owner(o)
    , tick(t)
    , func(std::move(f))
    , interval(i)
    {}
};

struct TimerCompare
{
    /// implement "less than"
    bool operator() (dumb_ptr<TimerData> l, dumb_ptr<TimerData> r)
    {
        // C++ provides a max-heap, but we want
        // the smallest tick to be the head (a min-heap).
        return l->tick > r->tick;
    }
};

static
std::priority_queue<dumb_ptr<TimerData>, std::vector<dumb_ptr<TimerData>>, TimerCompare> timer_heap;


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
void do_nothing(TimerData *, tick_t)
{
}

void Timer::cancel()
{
    if (!td)
        return;

    assert (this == td->owner);
    td->owner = nullptr;
    td->func = do_nothing;
    td->interval = interval_t::zero();
    td = nullptr;
}

void Timer::detach()
{
    assert (this == td->owner);
    td->owner = nullptr;
    td = nullptr;
}

static
void push_timer_heap(dumb_ptr<TimerData> td)
{
    timer_heap.push(td);
}

static
dumb_ptr<TimerData> top_timer_heap(void)
{
    if (timer_heap.empty())
        return dumb_ptr<TimerData>();
    return timer_heap.top();
}

static
void pop_timer_heap(void)
{
    timer_heap.pop();
}

Timer::Timer(tick_t tick, timer_func func, interval_t interval)
: td(dumb_ptr<TimerData>::make(this, tick, std::move(func), interval))
{
    assert (interval >= interval_t::zero());

    push_timer_heap(td);
}

Timer::Timer(Timer&& t)
: td(t.td)
{
    t.td = nullptr;
    if (td)
    {
        assert (td->owner == &t);
        td->owner = this;
    }
}

Timer& Timer::operator = (Timer&& t)
{
    std::swap(td, t.td);
    if (td)
    {
        assert (td->owner == &t);
        td->owner = this;
    }
    if (t.td)
    {
        assert (t.td->owner == this);
        t.td->owner = &t;
    }
    return *this;
}

interval_t do_timer(tick_t tick)
{
    /// Number of milliseconds until it calls this again
    // this says to wait 1 sec if all timers get popped
    interval_t nextmin = std::chrono::seconds(1);

    while (dumb_ptr<TimerData> td = top_timer_heap())
    {
        // while the heap is not empty and
        if (td->tick > tick)
        {
            /// Return the time until the next timer needs to goes off
            nextmin = td->tick - tick;
            break;
        }
        pop_timer_heap();

        // Prevent destroying the object we're in.
        // Note: this would be surprising in an interval timer,
        // but all interval timers do an immediate explicit detach().
        if (td->owner)
            td->owner->detach();
        // If we are too far past the requested tick, call with
        // the current tick instead to fix reregistration problems
        if (td->tick + std::chrono::seconds(1) < tick)
            td->func(td.operator->(), tick);
        else
            td->func(td.operator->(), td->tick);

        if (td->interval == interval_t::zero())
        {
            td.delete_();
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

tick_t file_modified(ZString name)
{
    struct stat buf;
    if (stat(name.c_str(), &buf))
        return tick_t();
    return tick_t(std::chrono::seconds(buf.st_mtime));
}

bool has_timers()
{
    return !timer_heap.empty();
}

#ifndef TIMER_HPP
#define TIMER_HPP

# include "timer.t.hpp"

# include "sanity.hpp"

# include <chrono>
# include <functional>

/// (to get additional arguments, use std::bind or a lambda).
typedef std::function<void (TimerData *, tick_t)> timer_func;

// updated automatically when using milli_clock::now()
// which is done only by core.cpp
extern tick_t gettick_cache;

inline
tick_t gettick(void)
{
    return gettick_cache;
}

/// Schedule a one-shot timer at the given tick.
/// The timer will automatically be freed after it is called
/// (during a do_timer).
TimerData *add_timer(tick_t t, timer_func f);

/// Schedule a recurring timer initially at the given tick.
/// The timer will automatically reregister itself, with the same
/// opaque handle, every interval after the tick.
/// It will never be freed unless you use delete_timer.
TimerData *add_timer_interval(tick_t, timer_func, interval_t);

/// Cancel the given timer.
/// This doesn't actually remove it, it just resets the functor.
/// and waits for the the tick to arrive in do_timer.
void delete_timer(TimerData *);

/// Do all timers scheduled before tick, and return the number of milliseconds until the next timer happens
interval_t do_timer(tick_t tick);

/// Stat a file, and return its modification time, truncated to seconds.
tick_t file_modified(const char *name);

#endif // TIMER_HPP

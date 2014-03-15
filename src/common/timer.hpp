#ifndef TIMER_HPP
#define TIMER_HPP

# include "timer.t.hpp"

# include "../sanity.hpp"

# include "../strings/fwd.hpp"

// updated automatically when using milli_clock::now()
// which is done only by core.cpp
extern tick_t gettick_cache;

inline
tick_t gettick(void)
{
    return gettick_cache;
}

/// Do all timers scheduled before tick, and return the number of
/// milliseconds until the next timer happens
interval_t do_timer(tick_t tick);

/// Stat a file, and return its modification time, truncated to seconds.
tick_t file_modified(ZString name);

/// Check if there are any events at all scheduled.
bool has_timers();

#endif // TIMER_HPP

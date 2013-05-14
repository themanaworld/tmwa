#ifndef TIMER_T_HPP
#define TIMER_T_HPP

# include <chrono>
# include <functional>

# include "dumb_ptr.hpp"

struct TimerData;

/// An implementation of the C++ "clock" concept, exposing
/// durations in milliseconds.
class milli_clock
{
public:
    typedef std::chrono::milliseconds duration;
    typedef duration::rep rep;
    typedef duration::period period;
    typedef std::chrono::time_point<milli_clock, duration> time_point;
    static const bool is_steady = true; // assumed - not necessarily true

    static time_point now() noexcept;
};

/// A point in time.
typedef milli_clock::time_point tick_t;
/// The difference between two points in time.
typedef milli_clock::duration interval_t;
/// (to get additional arguments, use std::bind or a lambda).
typedef std::function<void (TimerData *, tick_t)> timer_func;

class Timer
{
    friend struct TimerData;
    dumb_ptr<TimerData> td;

    Timer(const Timer&) = delete;
    Timer& operator = (const Timer&) = delete;
public:
    /// Don't own anything yet.
    Timer() = default;
    /// Schedule a timer for the given tick.
    /// If you do not wish to keep track of it, call disconnect().
    /// Otherwise, you may cancel() or replace (operator =) it later.
    ///
    /// If the interval argument is given, the timer will reschedule
    /// itself again forever. Otherwise, it will disconnect() itself
    /// just BEFORE it is called.
    Timer(tick_t tick, timer_func func, interval_t interval=interval_t::zero());

    Timer(Timer&& t);
    Timer& operator = (Timer&& t);
    ~Timer() { cancel(); }

    /// Cancel the delivery of this timer's function, and make it falsy.
    /// Implementation note: this doesn't actually remove it, just sets
    /// the functor to do_nothing, and waits for the tick before removing.
    void cancel();
    /// Make it falsy without cancelling the timer,
    void detach();

    /// Check if there is a timer connected.
    explicit operator bool() { return bool(td); }
    /// Check if there is no connected timer.
    bool operator !() { return !td; }
};

#endif // TIMER_T_HPP

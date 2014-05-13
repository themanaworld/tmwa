#ifndef TMWA_NET_TIMER_T_HPP
#define TMWA_NET_TIMER_T_HPP
//    timer.t.hpp - Future event scheduler.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

# include "fwd.hpp"

# include <chrono>
# include <functional>

# include "../generic/dumb_ptr.hpp"

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

#endif // TMWA_NET_TIMER_T_HPP

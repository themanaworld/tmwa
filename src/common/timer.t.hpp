#ifndef TIMER_T_HPP
#define TIMER_T_HPP

# include <chrono>

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

/// Opaque type representing an active timer.
struct TimerData;

#endif // TIMER_T_HPP

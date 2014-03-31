#ifndef TMWA_GENERIC_MATRIX_HPP
#define TMWA_GENERIC_MATRIX_HPP

# include "../sanity.hpp"

# include <cassert>

# include "../compat/memory.hpp"

template<class T>
class Matrix
{
    std::unique_ptr<T[]> _data;
    size_t _xs, _ys;
public:
    Matrix()
    : _data()
    , _xs()
    , _ys()
    {}
    Matrix(size_t x, size_t y)
    : _data(make_unique<T[]>(x * y))
    , _xs(x)
    , _ys(y)
    {}
    // no copy-ctor or copy-assign

    void reset(size_t x, size_t y)
    {
        *this = Matrix(x, y);
    }
    void clear()
    {
        *this = Matrix();
    }

    T& ref(size_t x, size_t y)
    {
        assert (x < _xs);
        assert (y < _ys);
        return _data[x + y * _xs];
    }
    const T& ref(size_t x, size_t y) const
    {
        assert (x < _xs);
        assert (y < _ys);
        return _data[x + y * _xs];
    }

    size_t xs() const
    {
        return _xs;
    }
    size_t ys() const
    {
        return _ys;
    }
};

#endif // TMWA_GENERIC_MATRIX_HPP

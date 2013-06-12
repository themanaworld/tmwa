#ifndef TMWA_COMMON_MATRIX_HPP
#define TMWA_COMMON_MATRIX_HPP

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
    : _data(new T[x * y]())
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
        return _data[x + y * _xs];
    }
    const T& ref(size_t x, size_t y) const
    {
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

#endif // TMWA_COMMON_MATRIX_HPP

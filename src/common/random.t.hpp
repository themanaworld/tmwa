#ifndef RANDOM_T_HPP
#define RANDOM_T_HPP

namespace random_
{
    struct Fraction
    {
        int num, den;
    };

    template<class T, T den>
    struct Fixed
    {
        T num;

        operator Fraction()
        {
            return {num, den};
        }
    };
} // namespace random_

#endif // RANDOM_T_HPP

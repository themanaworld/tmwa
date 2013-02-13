#include "random2.hpp"

#include "../poison.hpp"

namespace random_
{
    std::mt19937 generate{std::random_device()()};
} // namespace random_

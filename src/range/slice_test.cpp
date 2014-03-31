#include "slice.hpp"

#include <gtest/gtest.h>

TEST(slice, slice)
{
    int init[] = {1, 2, 3, 4, 5};

    Slice<int> slice(std::begin(init), std::end(init));
    EXPECT_EQ(slice.data(), init);
    EXPECT_EQ(slice.size(), 5);

    Slice<int> head = slice.slice_h(2);
    Slice<int> tail = slice.slice_t(2);
    EXPECT_EQ(head.size(), 2);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(head.front(), 1);
    EXPECT_EQ(head.back(), 2);
    EXPECT_EQ(tail.front(), 3);
    EXPECT_EQ(tail.back(), 5);

    head = slice.rslice_h(3);
    tail = slice.rslice_t(3);
    EXPECT_EQ(head.size(), 2);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(head.front(), 1);
    EXPECT_EQ(head.back(), 2);
    EXPECT_EQ(tail.front(), 3);
    EXPECT_EQ(tail.back(), 5);

    head = slice.islice_h(slice.begin() + 2);
    tail = slice.islice_t(slice.end() - 3);
    EXPECT_EQ(head.size(), 2);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(head.front(), 1);
    EXPECT_EQ(head.back(), 2);
    EXPECT_EQ(tail.front(), 3);
    EXPECT_EQ(tail.back(), 5);

    tail = slice.lslice(1, 3);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(tail.front(), 2);
    EXPECT_EQ(tail.back(), 4);

    tail = slice.pslice(1, 4);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(tail.front(), 2);
    EXPECT_EQ(tail.back(), 4);

    tail = slice.islice(slice.begin() + 1, slice.end() - 1);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(tail.front(), 2);
    EXPECT_EQ(tail.back(), 4);

    head = slice;
    while (head)
    {
        size_t headsize = head.size();
        EXPECT_EQ(head.back(), headsize);
        EXPECT_EQ(head.pop_back(), headsize);
    }

    tail = slice;
    while (!!tail)
    {
        size_t tailsize = tail.size();
        EXPECT_EQ(tail.front(), 6 - tailsize);
        EXPECT_EQ(tail.pop_front(), 6 - tailsize);
    }
}

TEST(slice, cast)
{
    struct Foo
    {
        int x;
    };
    struct Bar : Foo
    {
    };

    Bar bars[2] = {Bar(), Bar()};

    Slice<Bar> slice(bars, 2);
    Slice<Foo> foos(slice);

    EXPECT_EQ(foos.size(), slice.size());
    EXPECT_EQ(&foos.end()[-1], &slice.end()[-1]);
}

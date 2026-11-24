#include "ringbuf.hpp"

#include <gtest/gtest.h>

#include <cstdint>

// A test of a constexpr function can run in the compiler. A failure is a compile error.
constexpr bool wraps_after_three_pushes()
{
    RingBuffer<int, 3> rb;
    rb.push(1);
    rb.push(2);
    rb.push(3);
    const bool overwrote = rb.push(4);
    return overwrote && rb.count() == 3 && rb.get(0) == 2 && rb.get(2) == 4;
}
static_assert(wraps_after_three_pushes(), "the buffer must drop the oldest sample when full");

// The fixture plays the part of setUp and tearDown: each TEST_F gets a new object.
class RingBufferTest : public ::testing::Test
{
  protected:
    RingBuffer<std::int16_t, 4> rb;

    void fill(std::int16_t first, int n)
    {
        for (int i = 0; i < n; i++) {
            rb.push(static_cast<std::int16_t>(first + i));
        }
    }
};

TEST_F(RingBufferTest, NewBufferIsEmpty)
{
    EXPECT_EQ(rb.count(), 0u);
    EXPECT_FALSE(rb.get(0).has_value());
}

TEST_F(RingBufferTest, PushReportsNoOverwriteUntilFull)
{
    for (int i = 0; i < 4; i++) {
        EXPECT_FALSE(rb.push(static_cast<std::int16_t>(i))) << "push " << i;
    }
    EXPECT_TRUE(rb.push(4));
}

TEST_F(RingBufferTest, OverwriteDropsTheOldestSample)
{
    fill(10, 5);
    ASSERT_EQ(rb.count(), 4u);
    ASSERT_TRUE(rb.get(0).has_value());
    EXPECT_EQ(*rb.get(0), 11);
    EXPECT_EQ(*rb.get(3), 14);
}

TEST_F(RingBufferTest, GetPastTheEndIsEmpty)
{
    fill(1, 2);
    EXPECT_TRUE(rb.get(1).has_value());
    EXPECT_FALSE(rb.get(2).has_value());
}

// The same checks for every element type that the firmware stores.
template <typename T> class RingBufferTyped : public ::testing::Test
{
};
using Samples = ::testing::Types<std::uint8_t, std::int16_t, std::uint32_t, float>;
TYPED_TEST_SUITE(RingBufferTyped, Samples);

TYPED_TEST(RingBufferTyped, KeepsTheLastNSamplesInOrder)
{
    RingBuffer<TypeParam, 3> rb;
    for (int i = 1; i <= 7; i++) {
        rb.push(static_cast<TypeParam>(i));
    }
    ASSERT_EQ(rb.count(), 3u);
    EXPECT_EQ(rb.get(0), static_cast<TypeParam>(5));
    EXPECT_EQ(rb.get(1), static_cast<TypeParam>(6));
    EXPECT_EQ(rb.get(2), static_cast<TypeParam>(7));
}

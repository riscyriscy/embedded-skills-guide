#include "filter/median.h"
#include "platform.h"
#include "sensor/ringbuf.h"
#include "sensor/stats.h"
#include "unity.h"

static ringbuf_t rb;

void setUp(void)
{
    ringbuf_init(&rb);
}

void tearDown(void)
{
}

static void fill(int16_t first, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        (void)ringbuf_push(&rb, (int16_t)(first + (int16_t)i));
    }
}

void test_new_buffer_is_empty(void)
{
    int16_t v = 0;
    TEST_ASSERT_EQUAL_UINT(0, ringbuf_count(&rb));
    TEST_ASSERT_FALSE(ringbuf_get(&rb, 0, &v));
    TEST_ASSERT_EQUAL_INT16(0, stats_mean(&rb));
}

void test_push_reports_no_overwrite_until_full(void)
{
    for (size_t i = 0; i < RINGBUF_CAPACITY; i++) {
        TEST_ASSERT_FALSE(ringbuf_push(&rb, (int16_t)i));
    }
    TEST_ASSERT_EQUAL_UINT(RINGBUF_CAPACITY, ringbuf_count(&rb));
    TEST_ASSERT_TRUE(ringbuf_push(&rb, 100));
    TEST_ASSERT_EQUAL_UINT(RINGBUF_CAPACITY, ringbuf_count(&rb));
}

void test_overwrite_drops_the_oldest_sample(void)
{
    int16_t v = 0;
    fill(1, RINGBUF_CAPACITY);
    (void)ringbuf_push(&rb, 100);
    TEST_ASSERT_TRUE(ringbuf_get(&rb, 0, &v));
    TEST_ASSERT_EQUAL_INT16(2, v);
    TEST_ASSERT_TRUE(ringbuf_get(&rb, RINGBUF_CAPACITY - 1, &v));
    TEST_ASSERT_EQUAL_INT16(100, v);
}

void test_get_past_the_end_fails(void)
{
    int16_t v = 77;
    fill(1, 3);
    TEST_ASSERT_FALSE(ringbuf_get(&rb, 3, &v));
    TEST_ASSERT_EQUAL_INT16(77, v); /* the output stays unchanged */
}

void test_mean_of_a_full_buffer(void)
{
    fill(1, RINGBUF_CAPACITY); /* 1 to 16, sum 136 */
    TEST_ASSERT_EQUAL_INT16(8, stats_mean(&rb));
}

void test_median_rejects_a_single_spike(void)
{
    (void)ringbuf_push(&rb, 10);
    (void)ringbuf_push(&rb, 500);
    (void)ringbuf_push(&rb, 12);
    TEST_ASSERT_EQUAL_INT16(12, filter_median3(&rb));
}

void test_stored_samples_match_what_was_pushed(void)
{
    int16_t expected[4] = {5, 6, 7, 8};
    int16_t actual[4] = {0};
    fill(5, 4);
    for (size_t i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(ringbuf_get(&rb, i, &actual[i]));
    }
    TEST_ASSERT_EQUAL_INT16_ARRAY(expected, actual, 4);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_new_buffer_is_empty);
    RUN_TEST(test_push_reports_no_overwrite_until_full);
    RUN_TEST(test_overwrite_drops_the_oldest_sample);
    RUN_TEST(test_get_past_the_end_fails);
    RUN_TEST(test_mean_of_a_full_buffer);
    RUN_TEST(test_median_rejects_a_single_spike);
    RUN_TEST(test_stored_samples_match_what_was_pushed);
    plat_exit(UNITY_END());
}

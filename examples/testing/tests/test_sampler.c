#include "fake_adc.h"
#include "platform.h"
#include "sampler.h"
#include "sensor/stats.h"
#include "unity.h"

static sampler_t s;

void setUp(void)
{
    sampler_init(&s);
    fake_adc_set(0, true);
}

void tearDown(void)
{
}

void test_full_scale_reads_as_the_reference_voltage(void)
{
    int16_t mv = 0;
    fake_adc_set(4095, true);
    TEST_ASSERT_TRUE(sampler_poll(&s));
    TEST_ASSERT_TRUE(ringbuf_get(&s.samples, 0, &mv));
    TEST_ASSERT_EQUAL_INT16(3300, mv);
}

void test_zero_reads_as_zero_volts(void)
{
    int16_t mv = -1;
    TEST_ASSERT_TRUE(sampler_poll(&s));
    TEST_ASSERT_TRUE(ringbuf_get(&s.samples, 0, &mv));
    TEST_ASSERT_EQUAL_INT16(0, mv);
}

void test_two_readings_give_their_mean(void)
{
    fake_adc_set(0, true);
    (void)sampler_poll(&s);
    fake_adc_set(4095, true);
    (void)sampler_poll(&s);
    TEST_ASSERT_EQUAL_INT16(1650, stats_mean(&s.samples));
}

void test_a_converter_error_is_counted_and_stores_nothing(void)
{
    fake_adc_set(4095, false);
    TEST_ASSERT_FALSE(sampler_poll(&s));
    TEST_ASSERT_EQUAL_UINT(1, s.errors);
    TEST_ASSERT_EQUAL_UINT(0, ringbuf_count(&s.samples));
    TEST_ASSERT_EQUAL_UINT(1, fake_adc_calls());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_full_scale_reads_as_the_reference_voltage);
    RUN_TEST(test_zero_reads_as_zero_volts);
    RUN_TEST(test_two_readings_give_their_mean);
    RUN_TEST(test_a_converter_error_is_counted_and_stores_nothing);
    plat_exit(UNITY_END());
}

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "adc.h"
#include "sampler.h"

/* The mock has no behaviour of its own. Each call takes the values that the test queued with
   will_return, in order, and the test fails when the queue is not used up. */
bool adc_read(uint16_t *raw)
{
    *raw = mock_type(uint16_t);
    return mock_type(bool);
}

static void full_scale_reads_as_the_reference_voltage(void **state)
{
    (void)state;
    sampler_t s;
    int16_t mv = 0;
    sampler_init(&s);
    will_return(adc_read, 4095);
    will_return(adc_read, true);
    assert_true(sampler_poll(&s));
    assert_true(ringbuf_get(&s.samples, 0, &mv));
    assert_int_equal(3300, mv);
}

static void a_converter_error_is_counted(void **state)
{
    (void)state;
    sampler_t s;
    sampler_init(&s);
    will_return(adc_read, 0);
    will_return(adc_read, false);
    assert_false(sampler_poll(&s));
    assert_int_equal(1, s.errors);
    assert_int_equal(0, ringbuf_count(&s.samples));
}

#ifdef SHOW_UNUSED_EXPECTATION
static void an_unused_expectation_fails_the_test(void **state)
{
    (void)state;
    sampler_t s;
    sampler_init(&s);
    will_return(adc_read, 100);
    will_return(adc_read, true);
    will_return(adc_read, 200); /* the code under test reads the converter only once */
    will_return(adc_read, true);
    assert_true(sampler_poll(&s));
}
#endif

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(full_scale_reads_as_the_reference_voltage),
        cmocka_unit_test(a_converter_error_is_counted),
#ifdef SHOW_UNUSED_EXPECTATION
        cmocka_unit_test(an_unused_expectation_fails_the_test),
#endif
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

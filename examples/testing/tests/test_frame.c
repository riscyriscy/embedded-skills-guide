#include <string.h>

#include "frame.h"
#include "platform.h"
#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_crc_matches_the_published_check_value(void)
{
    const uint8_t text[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    TEST_ASSERT_EQUAL_HEX16(0xBB3D, frame_crc16(text, sizeof text));
}

void test_round_trip_of_a_short_payload(void)
{
    const uint8_t sent[3] = {1, 2, 3};
    uint8_t wire[FRAME_OVERHEAD + FRAME_MAX_PAYLOAD];
    uint8_t got[FRAME_MAX_PAYLOAD] = {0};
    size_t got_len = 0;

    size_t n = frame_encode(sent, sizeof sent, wire, sizeof wire);
    TEST_ASSERT_EQUAL_UINT(FRAME_OVERHEAD + 3, n);
    TEST_ASSERT_EQUAL_INT(FRAME_OK, frame_decode(wire, n, got, sizeof got, &got_len));
    TEST_ASSERT_EQUAL_UINT(3, got_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(sent, got, 3);
}

void test_round_trip_of_an_empty_payload(void)
{
    uint8_t wire[FRAME_OVERHEAD];
    uint8_t got[1];
    size_t got_len = 99;

    size_t n = frame_encode(NULL, 0, wire, sizeof wire);
    TEST_ASSERT_EQUAL_UINT(FRAME_OVERHEAD, n);
    TEST_ASSERT_EQUAL_INT(FRAME_OK, frame_decode(wire, n, got, sizeof got, &got_len));
    TEST_ASSERT_EQUAL_UINT(0, got_len);
}

void test_round_trip_of_the_longest_payload(void)
{
    uint8_t sent[FRAME_MAX_PAYLOAD];
    uint8_t wire[FRAME_OVERHEAD + FRAME_MAX_PAYLOAD];
    uint8_t got[FRAME_MAX_PAYLOAD] = {0};
    size_t got_len = 0;

    for (size_t i = 0; i < sizeof sent; i++) {
        sent[i] = (uint8_t)(0xF0u + i);
    }
    size_t n = frame_encode(sent, sizeof sent, wire, sizeof wire);
    TEST_ASSERT_EQUAL_UINT(sizeof wire, n);
    TEST_ASSERT_EQUAL_INT(FRAME_OK, frame_decode(wire, n, got, sizeof got, &got_len));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(sent, got, sizeof sent);
}

void test_encode_refuses_a_payload_that_is_too_long(void)
{
    uint8_t payload[FRAME_MAX_PAYLOAD + 1] = {0};
    uint8_t wire[64];
    TEST_ASSERT_EQUAL_UINT(0, frame_encode(payload, sizeof payload, wire, sizeof wire));
}

void test_decode_rejects_a_frame_that_is_too_short(void)
{
    const uint8_t wire[3] = {FRAME_SYNC, 0, 0};
    uint8_t got[4];
    size_t got_len = 0;
    TEST_ASSERT_EQUAL_INT(FRAME_ERR_SHORT,
                          frame_decode(wire, sizeof wire, got, sizeof got, &got_len));
}

void test_decode_rejects_a_wrong_sync_byte(void)
{
    uint8_t wire[FRAME_OVERHEAD];
    uint8_t got[4];
    size_t got_len = 0;
    size_t n = frame_encode(NULL, 0, wire, sizeof wire);
    wire[0] = 0x5A;
    TEST_ASSERT_EQUAL_INT(FRAME_ERR_SYNC, frame_decode(wire, n, got, sizeof got, &got_len));
}

void test_decode_rejects_a_damaged_payload(void)
{
    const uint8_t sent[2] = {10, 20};
    uint8_t wire[FRAME_OVERHEAD + 2];
    uint8_t got[4];
    size_t got_len = 0;
    size_t n = frame_encode(sent, sizeof sent, wire, sizeof wire);
    wire[2] ^= 0x01;
    TEST_ASSERT_EQUAL_INT(FRAME_ERR_CRC, frame_decode(wire, n, got, sizeof got, &got_len));
}

void test_decode_rejects_a_length_that_exceeds_the_data(void)
{
    uint8_t wire[FRAME_OVERHEAD + 2] = {FRAME_SYNC, 9, 0, 0, 0, 0};
    uint8_t got[16];
    size_t got_len = 0;
    TEST_ASSERT_EQUAL_INT(FRAME_ERR_SHORT,
                          frame_decode(wire, sizeof wire, got, sizeof got, &got_len));
}

/* The caller's buffer holds 16 bytes. A frame with a valid check value that announces 17 must be
   refused, because the copy would write one byte past the end. */
void test_decode_refuses_a_payload_one_byte_larger_than_the_buffer(void)
{
    uint8_t wire[FRAME_OVERHEAD + FRAME_MAX_PAYLOAD + 1] = {FRAME_SYNC, FRAME_MAX_PAYLOAD + 1};
    uint8_t got[FRAME_MAX_PAYLOAD];
    size_t got_len = 0;
    uint16_t crc = frame_crc16(&wire[1], 1u + FRAME_MAX_PAYLOAD + 1u);
    wire[2 + FRAME_MAX_PAYLOAD + 1] = (uint8_t)(crc & 0xFFu);
    wire[3 + FRAME_MAX_PAYLOAD + 1] = (uint8_t)(crc >> 8);
    TEST_ASSERT_EQUAL_INT_MESSAGE(FRAME_ERR_LENGTH,
                                  frame_decode(wire, sizeof wire, got, sizeof got, &got_len),
                                  "a payload larger than the buffer was accepted");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc_matches_the_published_check_value);
    RUN_TEST(test_round_trip_of_a_short_payload);
    RUN_TEST(test_round_trip_of_an_empty_payload);
    RUN_TEST(test_round_trip_of_the_longest_payload);
    RUN_TEST(test_encode_refuses_a_payload_that_is_too_long);
    RUN_TEST(test_decode_rejects_a_frame_that_is_too_short);
    RUN_TEST(test_decode_rejects_a_wrong_sync_byte);
    RUN_TEST(test_decode_rejects_a_damaged_payload);
    RUN_TEST(test_decode_rejects_a_length_that_exceeds_the_data);
    RUN_TEST(test_decode_refuses_a_payload_one_byte_larger_than_the_buffer);
    plat_exit(UNITY_END());
}

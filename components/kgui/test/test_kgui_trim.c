/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file test_kgui_trim.c
 * @brief Host tests of the spaces a typed name loses at both ends (kgui_trim.h).
 */
#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "kgui_trim.h"

#define NAME_MAX_TEST  15U  /**< The longest name a field holds: the Bluetooth name's. */
#define GUARD_BYTE     0x5AU  /**< What a buffer holds past its size before the call. */
#define GUARD_BYTES    8U     /**< How many bytes follow the buffer under test. */

/** @brief Unity's hook before each case: nothing to set. */
void setUp(void)
{
}

/** @brief Unity's hook after each case: nothing to put away. */
void tearDown(void)
{
}

/** @brief Trim a literal into a buffer of the name's size and check the length. */
static void expect_trim(const char *typed, const char *want)
{
    char         out[NAME_MAX_TEST + 1U];
    size_t const n = kgui_trim_spaces(typed, out, sizeof(out));

    TEST_ASSERT_EQUAL_STRING(want, out);
    TEST_ASSERT_EQUAL_UINT32(strlen(want), n);
}

void test_a_name_with_no_spaces_is_kept(void)
{
    expect_trim("ab", "ab");
}

void test_spaces_at_the_start_go(void)
{
    expect_trim("  ab", "ab");
}

/* The name the issue typed: a"b\c and one space last. */
void test_a_space_at_the_end_goes(void)
{
    expect_trim("a\"b\\c ", "a\"b\\c");
}

void test_spaces_at_both_ends_go(void)
{
    expect_trim("   ab   ", "ab");
}

void test_spaces_inside_stay(void)
{
    expect_trim("  a  b c ", "a  b c");
}

void test_a_name_of_spaces_is_empty(void)
{
    expect_trim("     ", "");
}

void test_an_empty_name_is_empty(void)
{
    expect_trim("", "");
}

void test_a_name_at_the_limit_is_kept_whole(void)
{
    expect_trim("abcdefghijklmno", "abcdefghijklmno");
}

void test_a_name_over_the_limit_is_cut_at_it(void)
{
    expect_trim("abcdefghijklmnopq", "abcdefghijklmno");
}

void test_the_spaces_left_at_the_cut_go_too(void)
{
    expect_trim("abcdefghijklmn  pq", "abcdefghijklmn");
}

void test_spaces_before_a_long_name_do_not_count_against_the_limit(void)
{
    expect_trim("   abcdefghijklmnopq", "abcdefghijklmno");
}

void test_nothing_is_written_past_the_buffer(void)
{
    char buf[NAME_MAX_TEST + 1U + GUARD_BYTES];
    size_t i;

    (void)memset(buf, (int)GUARD_BYTE, sizeof(buf));
    (void)kgui_trim_spaces("  abcdefghijklmnopqrstuvwxyz  ", buf, NAME_MAX_TEST + 1U);
    for ( i = NAME_MAX_TEST + 1U; i < sizeof(buf); i++ )
    {
        TEST_ASSERT_EQUAL_HEX8(GUARD_BYTE, (uint8_t)buf[i]);
    }
    TEST_ASSERT_EQUAL_STRING("abcdefghijklmno", buf);
}

void test_a_buffer_of_one_byte_holds_the_terminator(void)
{
    char out[1] = { 'x' };

    TEST_ASSERT_EQUAL_UINT32(0U, kgui_trim_spaces("ab", out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

void test_a_buffer_of_no_bytes_is_left_alone(void)
{
    char out[1] = { 'x' };

    TEST_ASSERT_EQUAL_UINT32(0U, kgui_trim_spaces("ab", out, 0U));
    TEST_ASSERT_EQUAL_CHAR('x', out[0]);
}

void test_no_words_give_an_empty_name(void)
{
    char out[NAME_MAX_TEST + 1U];

    (void)memset(out, (int)'x', sizeof(out));
    TEST_ASSERT_EQUAL_UINT32(0U, kgui_trim_spaces(NULL, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("", out);
}

void test_no_buffer_is_refused(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, kgui_trim_spaces("ab", NULL, NAME_MAX_TEST + 1U));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_a_name_with_no_spaces_is_kept);
    RUN_TEST(test_spaces_at_the_start_go);
    RUN_TEST(test_a_space_at_the_end_goes);
    RUN_TEST(test_spaces_at_both_ends_go);
    RUN_TEST(test_spaces_inside_stay);
    RUN_TEST(test_a_name_of_spaces_is_empty);
    RUN_TEST(test_an_empty_name_is_empty);
    RUN_TEST(test_a_name_at_the_limit_is_kept_whole);
    RUN_TEST(test_a_name_over_the_limit_is_cut_at_it);
    RUN_TEST(test_the_spaces_left_at_the_cut_go_too);
    RUN_TEST(test_spaces_before_a_long_name_do_not_count_against_the_limit);
    RUN_TEST(test_nothing_is_written_past_the_buffer);
    RUN_TEST(test_a_buffer_of_one_byte_holds_the_terminator);
    RUN_TEST(test_a_buffer_of_no_bytes_is_left_alone);
    RUN_TEST(test_no_words_give_an_empty_name);
    RUN_TEST(test_no_buffer_is_refused);
    return UNITY_END();
}

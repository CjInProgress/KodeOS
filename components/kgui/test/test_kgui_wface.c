/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file test_kgui_wface.c
 * @brief Host tests of whose face a write's card wears (kgui_wface.h).
 */
#include <string.h>

#include "unity.h"

#include "kgui_wface.h"

void setUp(void) {}
void tearDown(void) {}

void test_a_report_that_names_an_app_takes_its_face(void)
{
    kgui_wface_t w = { 0 };

    TEST_ASSERT_EQUAL_INT(KGUI_WFACE_TAKE, kgui_wface_report(&w, true, "snake"));
    TEST_ASSERT_TRUE(kgui_wface_is(&w, "snake"));
    TEST_ASSERT_TRUE_MESSAGE(w.owed, "a face just taken has its folder still to ask");
}

void test_the_same_app_again_changes_nothing(void)
{
    kgui_wface_t w = { 0 };

    (void)kgui_wface_report(&w, true, "snake");
    w.owed = false;                              /* its folder was asked */
    TEST_ASSERT_EQUAL_INT(KGUI_WFACE_KEEP, kgui_wface_report(&w, true, "snake"));
    TEST_ASSERT_FALSE_MESSAGE(w.owed, "every percent would ask the folder again");
    TEST_ASSERT_TRUE(kgui_wface_is(&w, "snake"));
}

void test_a_report_that_names_nobody_keeps_a_card_with_no_face(void)
{
    kgui_wface_t w = { 0 };

    TEST_ASSERT_EQUAL_INT(KGUI_WFACE_KEEP, kgui_wface_report(&w, true, ""));
    TEST_ASSERT_FALSE(kgui_wface_has(&w));
}

void test_another_app_takes_the_face_over(void)
{
    kgui_wface_t w = { 0 };

    (void)kgui_wface_report(&w, true, "ladder");   /* what the host called it */
    w.owed = false;
    TEST_ASSERT_EQUAL_INT(KGUI_WFACE_TAKE, kgui_wface_report(&w, true, "snake"));
    TEST_ASSERT_TRUE(kgui_wface_is(&w, "snake"));
    TEST_ASSERT_FALSE(kgui_wface_is(&w, "ladder"));
    TEST_ASSERT_TRUE(w.owed);
}

void test_a_write_that_is_no_app_drops_the_face(void)
{
    kgui_wface_t w = { 0 };

    (void)kgui_wface_report(&w, true, "snake");
    /* kodeOS or the radio, whatever its report carries. */
    TEST_ASSERT_EQUAL_INT(KGUI_WFACE_DROP, kgui_wface_report(&w, false, "snake"));
    TEST_ASSERT_FALSE(kgui_wface_has(&w));
    TEST_ASSERT_FALSE(w.owed);
    /* And with none to drop, nothing happens. */
    TEST_ASSERT_EQUAL_INT(KGUI_WFACE_KEEP, kgui_wface_report(&w, false, ""));
}

void test_an_app_report_with_no_name_drops_a_face_of_another_write(void)
{
    kgui_wface_t w = { 0 };

    (void)kgui_wface_report(&w, true, "snake");
    /* A write whose project is not known yet: it is not the one before. */
    TEST_ASSERT_EQUAL_INT(KGUI_WFACE_DROP, kgui_wface_report(&w, true, ""));
    TEST_ASSERT_FALSE(kgui_wface_has(&w));
}

void test_the_end_of_a_write_forgets_the_face(void)
{
    kgui_wface_t w = { 0 };

    (void)kgui_wface_report(&w, true, "snake");
    kgui_wface_end(&w);
    TEST_ASSERT_FALSE(kgui_wface_has(&w));
    TEST_ASSERT_FALSE(w.owed);
    /* The same app written again asks its folder again: it may have changed. */
    TEST_ASSERT_EQUAL_INT(KGUI_WFACE_TAKE, kgui_wface_report(&w, true, "snake"));
}

void test_an_answer_is_the_cards_only_by_its_project(void)
{
    kgui_wface_t w = { 0 };

    TEST_ASSERT_FALSE_MESSAGE(kgui_wface_is(&w, ""),
        "a busy answer names nobody, and a card with no face is nobody");
    (void)kgui_wface_report(&w, true, "snake");
    TEST_ASSERT_FALSE(kgui_wface_is(&w, ""));
    TEST_ASSERT_FALSE(kgui_wface_is(&w, "snak"));
    TEST_ASSERT_TRUE(kgui_wface_is(&w, "snake"));
}

void test_a_name_longer_than_a_project_is_cut_not_overrun(void)
{
    kgui_wface_t w;
    char         big[KGUI_WFACE_PROJECT_MAX + 8U];

    (void)memset(&w, 0, sizeof(w));
    (void)memset(big, 'x', sizeof(big) - 1U);
    big[sizeof(big) - 1U] = '\0';
    TEST_ASSERT_EQUAL_INT(KGUI_WFACE_TAKE, kgui_wface_report(&w, true, big));
    TEST_ASSERT_EQUAL_UINT(KGUI_WFACE_PROJECT_MAX - 1U, strlen(w.project));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_a_report_that_names_an_app_takes_its_face);
    RUN_TEST(test_the_same_app_again_changes_nothing);
    RUN_TEST(test_a_report_that_names_nobody_keeps_a_card_with_no_face);
    RUN_TEST(test_another_app_takes_the_face_over);
    RUN_TEST(test_a_write_that_is_no_app_drops_the_face);
    RUN_TEST(test_an_app_report_with_no_name_drops_a_face_of_another_write);
    RUN_TEST(test_the_end_of_a_write_forgets_the_face);
    RUN_TEST(test_an_answer_is_the_cards_only_by_its_project);
    RUN_TEST(test_a_name_longer_than_a_project_is_cut_not_overrun);
    return UNITY_END();
}

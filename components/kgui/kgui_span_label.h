/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file kgui_span_label.h
 * @brief A setting whose value is a length of time: the words the interface
 *        shows for it, and the step from one value to the next.
 */
#ifndef KGUI_SPAN_LABEL_H
#define KGUI_SPAN_LABEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A length of time as the interface shows it.
 *
 * The table covers the words `off_after` offers: `1h` is "1 hour", `6h`
 * "6 hours", `24h` "1 day", `3d` "3 days", `7d` "7 days" and `never`
 * "Never". A word the table does not have comes back as it was given.
 *
 * @param word The value as its setting's owner says it.
 * @return The English words; @p word itself when it has none; "" for NULL.
 */
const char *kgui_span_label(const char *word);

/**
 * @brief The value a step takes a setting to, round the words its owner offers.
 *
 * The words are the owner's, in the owner's order: the place returned is the
 * index a set of the setting carries.
 *
 * @param words The words the setting offers.
 * @param n     How many.
 * @param now   The word in force; one the list does not have, or NULL, steps
 *              to the first word going forward and to the last going back.
 * @param dir   -1 for the one before, anything else for the one after.
 * @return The place of the next word, or @p n when there is no list.
 */
uint8_t kgui_span_next(const char *const *words, uint8_t n, const char *now, int8_t dir);

#ifdef __cplusplus
}
#endif

#endif /* KGUI_SPAN_LABEL_H */

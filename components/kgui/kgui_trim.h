/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file kgui_trim.h
 * @brief A typed name without the spaces at its ends.
 *
 * Pure: no LVGL and no board. A name is what a person types in a keypad; the
 * spaces before and after it are not part of it, whoever reads the name back.
 */
#ifndef KGUI_TRIM_H
#define KGUI_TRIM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Copy what was typed without the spaces at either end.
 *
 * The spaces inside stay. At most @p size - 1 bytes are copied, counted from
 * the first byte that is no space; a name cut short loses the spaces the cut
 * left at its end too. @p out always ends in a terminator and nothing is
 * written past @p size.
 *
 * @param typed What was typed, terminated; NULL reads as "".
 * @param out   Receives the name; left as it was when NULL or when @p size is 0.
 * @param size  Bytes @p out holds, terminator included.
 * @return The bytes of the name, terminator not counted.
 */
size_t kgui_trim_spaces(const char *typed, char *out, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* KGUI_TRIM_H */

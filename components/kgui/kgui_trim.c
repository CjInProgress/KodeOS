/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file kgui_trim.c
 * @brief A typed name without the spaces at its ends (kgui_trim.h).
 */
#include "kgui_trim.h"

#include <string.h>

size_t kgui_trim_spaces(const char *typed, char *out, size_t size)
{
    size_t len = 0U;

    if ( (NULL != out) && (0U < size) )
    {
        if ( NULL != typed )
        {
            size_t skip = 0U;

            while ( ' ' == typed[skip] )
            {
                ++skip;
            }
            len = strlen(&typed[skip]);
            if ( len > (size - 1U) )
            {
                len = size - 1U;
            }
            while ( (0U < len) && (' ' == typed[skip + len - 1U]) )
            {
                --len;
            }
            (void)memcpy(out, &typed[skip], len);
        }
        out[len] = '\0';
    }
    return len;
}

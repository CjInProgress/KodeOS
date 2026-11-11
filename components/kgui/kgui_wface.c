/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file kgui_wface.c
 * @brief Whose face the card of a write wears (kgui_wface.h).
 */
#include "kgui_wface.h"

#include <stdio.h>
#include <string.h>

kgui_wface_act_t kgui_wface_report(kgui_wface_t *w, bool app, const char *project)
{
    kgui_wface_act_t act = KGUI_WFACE_KEEP;

    if ( !app || ('\0' == project[0]) )
    {
        if ( kgui_wface_has(w) )
        {
            kgui_wface_end(w);
            act = KGUI_WFACE_DROP;
        }
    }
    else if ( strncmp(w->project, project, sizeof(w->project) - 1U) != 0 )
    {
        (void)snprintf(w->project, sizeof(w->project), "%s", project);
        w->owed = true;
        act     = KGUI_WFACE_TAKE;
    }
    else
    {
        /* The app the card already wears. */
    }
    return act;
}

void kgui_wface_end(kgui_wface_t *w)
{
    w->project[0] = '\0';
    w->owed       = false;
}

bool kgui_wface_has(const kgui_wface_t *w)
{
    return '\0' != w->project[0];
}

bool kgui_wface_is(const kgui_wface_t *w, const char *project)
{
    return kgui_wface_has(w) && (strcmp(w->project, project) == 0);
}

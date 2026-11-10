/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file kgui_wface.h
 * @brief Whose face the card of a write wears when the write is about no row.
 *
 * A write this launcher did not start (a console `install`, the Kode channel)
 * is about no row of Apps, and its card used to wear the grey disc. The
 * writer's reports name the project being written, and an app's data folder
 * is named for its project and may be on the board before the app is: the
 * card wears that folder's name, icon and colour. This is the part that
 * decides, from each report, whether the card keeps the face it has, drops
 * it, or takes a new one; the face itself and its folder's question are the
 * launcher's.
 */
#ifndef KGUI_WFACE_H
#define KGUI_WFACE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KGUI_WFACE_PROJECT_MAX  32U  /**< A project name, terminator included: KODE_APP_PROJECT_MAX. */

/** @brief What a report does to the card's face. */
typedef enum {
    KGUI_WFACE_KEEP = 0,  /**< Nothing: the same app, or still nobody.           */
    KGUI_WFACE_DROP,      /**< The card is about no app any more: the face goes. */
    KGUI_WFACE_TAKE       /**< Another app: its face is to be taken.             */
} kgui_wface_act_t;

/** @brief Whose face the card wears. */
typedef struct {
    char project[KGUI_WFACE_PROJECT_MAX];  /**< The app's project name; "" for nobody. */
    bool owed;                             /**< Its folder is still to be asked.       */
} kgui_wface_t;

/**
 * @brief A report of a write arrived: say what it does to the card's face.
 *
 * @param w       The card's face.
 * @param app     The write is an app's; false for kodeOS and the radio.
 * @param project The project the report names; "" when it names none.
 * @return KGUI_WFACE_TAKE for an app other than the one the card wears, with
 *         @c owed set; KGUI_WFACE_DROP when the card wore a face and the
 *         report is no app's or names none; KGUI_WFACE_KEEP otherwise.
 */
kgui_wface_act_t kgui_wface_report(kgui_wface_t *w, bool app, const char *project);

/**
 * @brief The write ended: the card's face is nobody's.
 *
 * @param w The card's face.
 */
void kgui_wface_end(kgui_wface_t *w);

/**
 * @brief Whether the card wears a face.
 *
 * @param w The card's face.
 * @return true when a report has named an app and the write has not ended.
 */
bool kgui_wface_has(const kgui_wface_t *w);

/**
 * @brief Whether a folder's answer is about the card's face.
 *
 * @param w       The card's face.
 * @param project The project the answer names.
 * @return true when the card wears a face and it is that project's.
 */
bool kgui_wface_is(const kgui_wface_t *w, const char *project);

#ifdef __cplusplus
}
#endif

#endif /* KGUI_WFACE_H */

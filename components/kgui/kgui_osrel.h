/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file kgui_osrel.h
 * @brief The kodeOS release the update service names: reading its answer,
 *        telling a newer version, and what About's row says about it.
 *
 * Pure: no LVGL and no network. AO_Gui asks the service when About opens
 * and feeds what it hears; everything decided about the answer is decided
 * here.
 */
#ifndef KGUI_OSREL_H
#define KGUI_OSREL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- The numbers of an answer ----------------------------------------------- */

#define KGUI_OSREL_VERSION_MAX  16U        /**< A version, terminator included: three groups of up to four digits and two dots. */
#define KGUI_OSREL_GROUPS       3U         /**< Groups of digits in a version. */
#define KGUI_OSREL_GROUP_DIGITS 4U         /**< The most digits one group has. */
#define KGUI_OSREL_URL_MAX      128U       /**< The link of a release's file, terminator included. */
#define KGUI_OSREL_ANSWER_MAX   512U       /**< The most of the service's answer that is read, in bytes. */
#define KGUI_OSREL_TEXT_MAX     48U        /**< The row's words, terminator included. */
#define KGUI_OSREL_MIN_PCT      20U        /**< Under this charge, with no cable, an update is not started. */

/** @brief The release the service names. */
typedef struct {
    char version[KGUI_OSREL_VERSION_MAX];  /**< Its version, as the image says it: no `v`. */
    char url[KGUI_OSREL_URL_MAX];          /**< Where its `.kfw` is. */
} kgui_osrel_t;

/** @brief Where the check stands. */
typedef enum {
    KGUI_OSREL_NEVER = 0,   /**< The service has not been asked since the boot: opening About asks. */
    KGUI_OSREL_ASKING,      /**< A question is on its way. */
    KGUI_OSREL_CURRENT,     /**< The service names nothing newer than what runs. */
    KGUI_OSREL_AVAILABLE,   /**< The service names a newer version. */
    KGUI_OSREL_FAILED       /**< The last question got no usable answer. */
} kgui_osrel_check_t;

/** @brief What About's "Software update" row says. */
typedef enum {
    KGUI_OSREL_ROW_NONE = 0,   /**< No card image and no service in this build. */
    KGUI_OSREL_ROW_CARD,       /**< The card carries kodeos.bin. */
    KGUI_OSREL_ROW_AVAILABLE,  /**< A newer version can be installed now. */
    KGUI_OSREL_ROW_CHARGE,     /**< A newer version waits for more battery or a cable. */
    KGUI_OSREL_ROW_CURRENT,    /**< Nothing newer. */
    KGUI_OSREL_ROW_CHECKING,   /**< The answer is on its way. */
    KGUI_OSREL_ROW_OFFLINE,    /**< No network to ask with. */
    KGUI_OSREL_ROW_FAILED      /**< The service did not answer. */
} kgui_osrel_row_t;

/** @brief What the row is decided from. */
typedef struct {
    bool               service;  /**< This build has a service to ask. */
    bool               card;     /**< The card carries kodeos.bin. */
    bool               linked;   /**< Wi-Fi has an address. */
    bool               power;    /**< kgui_osrel_power_ok() said yes. */
    kgui_osrel_check_t check;    /**< Where the check stands. */
} kgui_osrel_view_t;

/**
 * @brief Read the service's answer.
 *
 * The answer is one JSON object; `version` and `url` are taken from its top
 * level and everything else is skipped. The version must be three groups of
 * digits and the link `https://`, printable, with no quote, backslash or
 * space in it.
 *
 * @param json     The answer; it need not end in a terminator.
 * @param len      Its bytes.
 * @param plain_ok Take an `http://` link too: only a build whose service is
 *                 itself asked over `http://`, on a bench.
 * @param out      Receives the release; left as it was on a refusal.
 * @return true when both fields were there and well formed.
 */
bool kgui_osrel_parse(const char *json, size_t len, bool plain_ok, kgui_osrel_t *out);

/**
 * @brief Whether @p latest is a newer version than @p running.
 *
 * Group by group, as numbers: `0.1.0` is newer than `00.00.24`, and
 * `00.01.00` is `0.1.0`. A version that is not three groups of digits is
 * never newer and nothing is newer than it.
 *
 * @param latest  The version the service names.
 * @param running The version that runs.
 * @return true only when both read and @p latest is the greater.
 */
bool kgui_osrel_newer(const char *latest, const char *running);

/**
 * @brief Whether the board has the power to start an update.
 * @param soc  The charge left, in percent.
 * @param vbus A cable is in.
 * @return true on a cable, or from KGUI_OSREL_MIN_PCT up.
 */
bool kgui_osrel_power_ok(uint8_t soc, bool vbus);

/**
 * @brief What the row says.
 * @param view What is known; NULL answers KGUI_OSREL_ROW_NONE.
 * @return The row's state.
 */
kgui_osrel_row_t kgui_osrel_row(const kgui_osrel_view_t *view);

/**
 * @brief Whether the top button on the row installs something.
 * @param row The row's state.
 * @return true for the card's image and for a version that can be installed now.
 */
bool kgui_osrel_row_installs(kgui_osrel_row_t row);

/**
 * @brief Whether the top button on the row asks the service again.
 * @param row The row's state.
 * @return true when the row shows an answer or a failure, and none is on its way.
 */
bool kgui_osrel_row_asks(kgui_osrel_row_t row);

/**
 * @brief Write the row's words.
 * @param row     The row's state.
 * @param version The newer version, for the two states that name it; NULL reads as "".
 * @param out     Receives the words and a terminator.
 * @param size    Bytes @p out holds; KGUI_OSREL_TEXT_MAX fits every state.
 */
void kgui_osrel_row_text(kgui_osrel_row_t row, const char *version, char *out, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* KGUI_OSREL_H */

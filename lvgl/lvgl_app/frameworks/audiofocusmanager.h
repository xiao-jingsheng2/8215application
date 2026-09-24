/**
 * @file audiofocusmanager.h
 * @brief Audio focus manager with stack-based model
 *
 * Focus State (audio_focus_state_t):
 * - FOCUS_NONE: No focus (used for initialization/release)
 * - FOCUS_GRANTED: Permanent focus (current owner)
 * - FOCUS_GRANTED_TRANSIENT: Temporary focus, displaced owner will be restored on release
 * - FOCUS_GRANTED_TRANSIENT_MAY_DUCK: Temporary focus, displaced owner may duck
 * - FOCUS_LOST: Permanent loss (displaced owner removed from stack)
 * - FOCUS_LOST_TRANSIENT: Temporary loss (displaced owner stays in stack)
 * - FOCUS_LOST_TRANSIENT_MAY_DUCK: Lost but can continue at low volume
 */

#ifndef AUDIOFOCUSMANAGER_H
#define AUDIOFOCUSMANAGER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=================== LOG MACRO ===================*/
#ifndef AUDIO_LOG
#include <stdio.h>
#define AUDIO_LOG(fmt, ...) \
    do { \
        printf("[AUDIO_FOCUS][%s:%d] " fmt "\r\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while(0)
#endif

#ifndef AUDIO_LOGI
#define AUDIO_LOGI(fmt, ...) AUDIO_LOG("[I] " fmt, ##__VA_ARGS__)
#endif

#ifndef AUDIO_LOGW
#define AUDIO_LOGW(fmt, ...) AUDIO_LOG("[W] " fmt, ##__VA_ARGS__)
#endif

#ifndef AUDIO_LOGE
#define AUDIO_LOGE(fmt, ...) AUDIO_LOG("[E] " fmt, ##__VA_ARGS__)
#endif

#ifndef AUDIO_LOGD
#define AUDIO_LOGD(fmt, ...) AUDIO_LOG("[D] " fmt, ##__VA_ARGS__)
#endif

/*=================== RETURN CODES ===================*/
#define AUDIO_OK         0   /**< Success */
#define AUDIO_ERR_INIT  -1  /**< Not initialized */
#define AUDIO_ERR_PARAM -2  /**< Invalid parameter */
#define AUDIO_ERR_BUSY  -3  /**< Request rejected: insufficient priority */
#define AUDIO_ERR_FULL  -4  /**< Capacity full */

/*=================== FOCUS TYPES ===================*/
typedef enum {
    FOCUS_DEFAULT = 0,           /**< Default type */
    FOCUS_MUSIC,                 /**< Music playback */
    FOCUS_NOTIFICATION,          /**< Notification/tone */
    FOCUS_NAVIGATION,            /**< Navigation/Guidance */
    FOCUS_SPEECH,                /**< Speech/TTS */
    FOCUS_CALL,                  /**< Phone call */
    FOCUS_TYPE_MAX
} audio_focus_type_t;

/*=================== FOCUS STATES ===================*/
typedef enum {
    FOCUS_NONE = 0,                        /**< No focus */
    FOCUS_GRANTED,                         /**< Permanent focus (current owner) */
    FOCUS_GRANTED_TRANSIENT,               /**< Temporary focus, displaced will be restored */
    FOCUS_GRANTED_TRANSIENT_MAY_DUCK,      /**< Temporary focus, displaced may duck */
    FOCUS_LOST,                            /**< Permanent loss */
    FOCUS_LOST_TRANSIENT,                  /**< Temporary loss */
    FOCUS_LOST_TRANSIENT_MAY_DUCK,         /**< Lost but can continue at low volume */
} audio_focus_state_t;

/*=================== UNIFIED FOCUS INFO ===================*/
typedef struct {
    int id;                       /**< Request ID */
    audio_focus_type_t focus_type;/**< Focus type */
    audio_focus_state_t state;    /**< Focus state */
} audio_focus_info_t;

/** Focus request (alias of audio_focus_info_t) */
typedef audio_focus_info_t audio_focus_request_t;

/** Focus change notification (alias of audio_focus_info_t) */
typedef audio_focus_info_t audio_focus_change_info_t;

/** Focus owner info (alias of audio_focus_info_t) */
typedef audio_focus_info_t focus_owner_info_t;

/*=================== CALLBACK ===================*/
/**
 * @brief Focus change callback
 * @param info Focus change information
 * @param user_data User data passed during registration
 */
typedef void (*audio_focus_change_callback_t)(const audio_focus_change_info_t *info,
                                               void *user_data);

/*=================== PUBLIC API ===================*/

/**
 * @brief Initialize audio focus manager
 */
int audiofocusmanager_init(void);

/**
 * @brief Deinitialize audio focus manager
 */
void audiofocusmanager_deinit(void);

/**
 * @brief Request audio focus (unique entry + owner on stack top)
 *
 * @param request Focus request parameters
 * @return AUDIO_OK, AUDIO_ERR_INIT, AUDIO_ERR_PARAM,
 *         AUDIO_ERR_BUSY, or AUDIO_ERR_FULL
 */
int audiofocusmanager_request_focus(const audio_focus_request_t *request);

/**
 * @brief Release audio focus by id (pop from stack)
 *
 * @param id Request ID to release (must match the id that acquired the focus)
 */
void audiofocusmanager_release_focus(int id);

/**
 * @brief Get current focus owner info
 * @param info Pointer to struct to fill with owner info
 * @return true if owner exists, false if no owner
 */
bool audiofocusmanager_get_owner_info(focus_owner_info_t *info);

/**
 * @brief Register focus change callback
 * @param id Callback ID to register callback for (matches request id)
 * @param callback Callback function
 * @param user_data User data passed to callback
 */
void audiofocusmanager_register_focus_callback(int id,
                                          audio_focus_change_callback_t callback,
                                          void *user_data);

/**
 * @brief Unregister focus callback
 * @param id Callback ID to unregister callback for
 */
void audiofocusmanager_unregister_focus_callback(int id);

/**
 * @brief Print current focus stack status (for debugging)
 */
void audiofocusmanager_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIOFOCUSMANAGER_H */

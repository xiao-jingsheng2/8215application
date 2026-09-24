/**
 * @file app_common.h
 * @brief Common APP base definitions shared across all modules
 *
 * Zero-dependency layer providing APP ID, priority, and return code enums
 * used by videofocusmanager, audiofocusmanager, and future modules.
 */

#ifndef APP_COMMON_H
#define APP_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/*=================== RETURN CODES ===================*/
#define APP_OK         0   /**< Success */
#define APP_ERR_INIT  -1  /**< Not initialized */
#define APP_ERR_PARAM -2  /**< Invalid parameter */
#define APP_ERR_FULL  -3  /**< Capacity full */
#define APP_ERR_BUSY  -4  /**< Resource busy / rejected */

/*=================== APP ID ===================*/
typedef enum {
    APP_NONE = 0,
    APP_HOME,                 /**< Home / Launcher */
    APP_CARPLAY,              /**< CarPlay */
    APP_CARPLAY_SETTINGS,     /**< CarPlay Settings */
    APP_ANDROID_AUTO,         /**< AndroidAuto */
    APP_ANDROID_AUTO_SETTINGS,/**< AndroidAuto Settings */
    APP_VIDEO,                /**< Video */
    APP_RVC,                  /**< Rear view camera */
    APP_MUSIC,                /**< Music */
    APP_SETTINGS,             /**< Settings */
    APP_BLUETOOTH,            /**< Bluetooth */
    APP_PICTURE,              /**< Picture browser */
    APP_CUSTOM_BASE = 100,    /**< Reserved for dynamic extensions */
} app_id_t;

/*=================== APP PRIORITY ===================*/
typedef enum {
    APP_PRIORITY_BACKGROUND = 0,  /**< Background, cannot interrupt any APP */
    APP_PRIORITY_LOW,             /**< Low priority */
    APP_PRIORITY_MEDIUM,          /**< Medium priority (default) */
    APP_PRIORITY_HIGH,            /**< High priority */
    APP_PRIORITY_SYSTEM,          /**< System level, always preemptible */
} app_priority_t;

/*=================== UTILITY FUNCTIONS ===================*/
const char *app_id_to_str(app_id_t id);
const char *app_priority_to_str(app_priority_t priority);

#ifdef __cplusplus
}
#endif

#endif /* APP_COMMON_H */

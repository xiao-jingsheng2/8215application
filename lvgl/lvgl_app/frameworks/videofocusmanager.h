/**
 * @file videofocusmanager.h
 * @brief Video focus manager with LIFO stack model for APP display management
 *
 * ================== STACK MODEL (LIFO) ==================
 * - open_app(): Push to top of stack, load LVGL screen
 * - close_current_app(): Pop from top, auto-restore previous screen
 * - close_app(): Remove arbitrary app from stack, destroy its screen
 * - go_home(): Reset stack to HOME, destroy all apps above, load HOME screen
 *
 * ================== PRIORITY GUARD ==================
 * - New APP priority must be >= current APP priority to open (SYSTEM never blocks)
 * - HOME (SYSTEM priority) is always at stack[0], never popped
 */

#ifndef VIDEOFOCUSMANAGER_H
#define VIDEOFOCUSMANAGER_H

#include <stdbool.h>
#include "app_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=================== LOG MACRO ===================*/
#ifndef VIDEO_LOG
#include <stdio.h>
#define VIDEO_LOG(fmt, ...) \
    do { \
        printf("[VIDEO_FOCUS][%s:%d] " fmt "\r\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while(0)
#endif

#ifndef VIDEO_LOGI
#define VIDEO_LOGI(fmt, ...) VIDEO_LOG("[I] " fmt, ##__VA_ARGS__)
#endif

#ifndef VIDEO_LOGW
#define VIDEO_LOGW(fmt, ...) VIDEO_LOG("[W] " fmt, ##__VA_ARGS__)
#endif

#ifndef VIDEO_LOGE
#define VIDEO_LOGE(fmt, ...) VIDEO_LOG("[E] " fmt, ##__VA_ARGS__)
#endif

#ifndef VIDEO_LOGD
#define VIDEO_LOGD(fmt, ...) VIDEO_LOG("[D] " fmt, ##__VA_ARGS__)
#endif

/*=================== FORWARD DECLARATION ===================*/
struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;

/*=================== LIFECYCLE CALLBACK TYPES ===================*/

/**
 * @brief APP initialization callback
 *
 * Called when an APP is opened for the first time. The callback must
 * create an LVGL screen via lv_obj_create(NULL) and store it in *screen.
 *
 * @param screen Output pointer to store the created LVGL screen object
 */
typedef void (*app_init_cb_t)(lv_obj_t **screen);

/**
 * @brief APP destroy callback
 *
 * Called when an APP is closed. Use this to clean up non-LVGL resources
 * (timers, tasks, etc.). The manager calls lv_obj_del() on the screen
 * after this callback returns.
 */
typedef void (*app_destroy_cb_t)(void);

/**
 * @brief Focus change callback
 *
 * Called whenever an APP gains or loses display focus.
 *
 * @param app_id   The APP whose focus changed
 * @param focused  true = gained focus, false = lost focus
 * @param user_data User data passed during callback registration
 */
typedef void (*app_focus_change_cb_t)(app_id_t app_id, bool focused, void *user_data);

/*=================== REGISTRATION STRUCT ===================*/

/**
 * @brief APP registration descriptor
 */
typedef struct {
    app_id_t app_id;              /**< APP ID (from app_common.h) */
    app_priority_t priority;      /**< Priority level */
    const char *name;             /**< Display name (for logging) */
    app_init_cb_t init_cb;        /**< Called on first open to create LVGL screen */
    app_destroy_cb_t destroy_cb;  /**< Called on close for cleanup */
} app_desc_t;

/*=================== PUBLIC API ===================*/

/**
 * @brief Initialize the video focus manager
 * @return APP_OK
 */
int videofocusmanager_init(void);

/**
 * @brief Deinitialize the video focus manager
 *
 * Destroys all screens, notifies callbacks with focused=false, resets state.
 */
void videofocusmanager_deinit(void);

/**
 * @brief Register an APP with its lifecycle callbacks
 * @param desc APP descriptor (copied internally)
 * @return APP_OK, APP_ERR_INIT, APP_ERR_PARAM, or APP_ERR_FULL
 */
int videofocusmanager_register_app(const app_desc_t *desc);

/**
 * @brief Update the runtime priority of a registered APP
 *
 * This does not reorder the current stack. The new priority is used by
 * subsequent open_app() priority checks.
 *
 * @param app_id APP to update
 * @param priority New priority
 * @return APP_OK, APP_ERR_INIT, or APP_ERR_PARAM
 */
int videofocusmanager_set_app_priority(app_id_t app_id, app_priority_t priority);

/**
 * @brief Open an APP (create if needed, then push or move it to the top)
 *
 * If not yet initialized, init_cb is called to create the LVGL screen.
 * Priority check: new priority >= current priority, else APP_ERR_BUSY.
 * Re-opening an APP already in the stack moves the existing APP to the top.
 *
 * @param app_id APP to open
 * @return APP_OK, APP_ERR_INIT, APP_ERR_PARAM, APP_ERR_FULL, or APP_ERR_BUSY
 */
int videofocusmanager_open_app(app_id_t app_id);

/**
 * @brief Close the current APP and restore the previous one
 *
 * HOME (stack[0]) is protected and can never be closed.
 * @return APP_OK, APP_ERR_INIT, or APP_ERR_PARAM (only HOME remains)
 */
int videofocusmanager_close_current_app(void);

/**
 * @brief Close a specific APP anywhere in the stack
 *
 * Finds and removes the APP, destroys its screen. If it was the current
 * top, the new top's screen is loaded.
 *
 * @param app_id APP to close
 * @return APP_OK, APP_ERR_INIT, or APP_ERR_PARAM
 */
int videofocusmanager_close_app(app_id_t app_id);

/**
 * @brief Navigate directly to the HOME screen
 *
 * Destroys all apps above HOME in the stack (indices &gt; 0),
 * resets the stack so HOME is the only entry, and loads HOME's screen.
 * Safe to call when only HOME is on the stack (no-op).
 * Safe to call when the stack is empty (HOME will be initialized and pushed).
 *
 * @return APP_OK, APP_ERR_INIT, or APP_ERR_PARAM (HOME not registered)
 */
int videofocusmanager_go_home(void);

/**
 * @brief Get the currently displayed APP ID
 * @return Current APP ID, or APP_NONE if not initialized or stack empty
 */
app_id_t videofocusmanager_get_current_app(void);

/**
 * @brief Get the current stack depth
 * @return Number of APPs in the stack, or -1 if not initialized
 */
int videofocusmanager_get_stack_size(void);

/**
 * @brief Check whether an APP is currently present in the navigation stack
 * @param app_id APP ID to query
 * @return true if present in stack, false otherwise
 */
bool videofocusmanager_is_in_stack(app_id_t app_id);

/**
 * @brief Register a per-APP focus change callback
 *
 * The callback is invoked only when the specified APP gains or loses display focus.
 * This is a per-APP registration (unlike the old broadcast model).
 * Only one callback per app_id is supported; registering again updates the existing one.
 *
 * @param app_id    APP ID to observe (notified only when this APP's focus changes)
 * @param cb        Callback function
 * @param user_data User data passed to callback
 */
void videofocusmanager_register_focus_callback(app_id_t app_id, app_focus_change_cb_t cb, void *user_data);

/**
 * @brief Unregister a per-APP focus change callback
 *
 * @param app_id APP ID whose callback should be removed
 */
void videofocusmanager_unregister_focus_callback(app_id_t app_id);

/**
 * @brief Print current stack state (for debugging)
 */
void videofocusmanager_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* VIDEOFOCUSMANAGER_H */

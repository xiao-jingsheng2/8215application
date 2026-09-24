/**
 * @file videofocusmanager.c
 * @brief Video focus manager with a unique APP stack model
 *
 * ================== STACK MODEL (LIFO) ==================
 * - Every APP appears at most once in the stack
 * - The current APP is always stack[top]
 * - open_app(): create if needed, then push or move-to-top
 * - close_current_app(): pop top, destroy it, restore previous APP
 * - go_home(): destroy everything above HOME and restore HOME
 */

#include "videofocusmanager.h"
#include "lvgl/lvgl.h"
#include <string.h>

/*=================== CONSTANTS ===================*/
#define MAX_APPS         32
#define MAX_STACK_DEPTH  32
#define MAX_FOCUS_CBS    8

/*=================== INTERNAL TYPES ===================*/
typedef struct {
    app_id_t app_id;
    app_priority_t priority;
    const char *name;
    app_init_cb_t init_cb;
    app_destroy_cb_t destroy_cb;
    lv_obj_t *screen;
    bool initialized;
} app_reg_t;

typedef struct {
    app_id_t app_id;
    app_focus_change_cb_t cb;
    void *user_data;
} focus_cb_entry_t;

typedef struct {
    app_reg_t apps[MAX_APPS];
    int app_count;
    app_id_t stack[MAX_STACK_DEPTH];
    int stack_size;
    focus_cb_entry_t focus_cbs[MAX_FOCUS_CBS];
    int focus_cb_count;
    bool initialized;
} videofocus_manager_t;

/*=================== STATIC STATE ===================*/
static videofocus_manager_t g_vm = {0};

/*=================== INTERNAL HELPERS ===================*/

static app_reg_t* find_app_by_id(app_id_t app_id)
{
    for (int i = 0; i < g_vm.app_count; i++) {
        if (g_vm.apps[i].app_id == app_id) {
            return &g_vm.apps[i];
        }
    }
    return NULL;
}

static int find_in_stack(app_id_t app_id)
{
    for (int i = 0; i < g_vm.stack_size; i++) {
        if (g_vm.stack[i] == app_id) {
            return i;
        }
    }
    return -1;
}

static app_reg_t* get_current_app_reg(void)
{
    if (g_vm.stack_size == 0) {
        return NULL;
    }
    return find_app_by_id(g_vm.stack[g_vm.stack_size - 1]);
}

static void notify_focus_change(app_id_t app_id, bool focused)
{
    for (int i = 0; i < g_vm.focus_cb_count; i++) {
        if (g_vm.focus_cbs[i].cb && g_vm.focus_cbs[i].app_id == app_id) {
            g_vm.focus_cbs[i].cb(app_id, focused, g_vm.focus_cbs[i].user_data);

            VIDEO_LOGD("notify_focus_change app id= %d, focused state = %d", (int)app_id, focused);
            return;
        }
    }
}

static int ensure_app_initialized(app_reg_t *app)
{
    lv_obj_t *screen = NULL;

    if (!app) {
        return APP_ERR_PARAM;
    }
    if (app->initialized && app->screen) {
        return APP_OK;
    }
    if (!app->init_cb) {
        VIDEO_LOGE("Null init_cb for app id=%d", (int)app->app_id);
        return APP_ERR_PARAM;
    }

    app->init_cb(&screen);
    if (!screen) {
        VIDEO_LOGE("init_cb for %s(id=%d) returned NULL screen", app->name, (int)app->app_id);
        return APP_ERR_PARAM;
    }

    app->screen = screen;
    app->initialized = true;
    return APP_OK;
}

static void destroy_app_resources(app_reg_t *app)
{
    
    if (!app || !app->initialized) {
        VIDEO_LOGW("destory app is null or not initialized");
        return;
    }

    VIDEO_LOGD("destory %s", app->name);

    if (app->destroy_cb) {
        VIDEO_LOGD("call %s destroy_cb", app->name);
        app->destroy_cb();
    }

    if (app->screen) {
        VIDEO_LOGD("delete %s screen", app->name);
        lv_obj_del(app->screen);
    }

    app->screen = NULL;
    app->initialized = false;
}

static void remove_stack_pos(int pos)
{
    if (pos < 0 || pos >= g_vm.stack_size) {
        return;
    }

    for (int i = pos; i < g_vm.stack_size - 1; i++) {
        g_vm.stack[i] = g_vm.stack[i + 1];
    }
    g_vm.stack_size--;
}

static void push_stack(app_id_t app_id)
{
    g_vm.stack[g_vm.stack_size] = app_id;
    g_vm.stack_size++;
}

static void move_stack_entry_to_top(int pos)
{
    app_id_t app_id;

    if (pos < 0 || pos >= g_vm.stack_size - 1) {
        return;
    }

    app_id = g_vm.stack[pos];
    remove_stack_pos(pos);
    push_stack(app_id);
}

static void load_app_screen(app_reg_t *app)
{
    if (!app || !app->screen) {
        return;
    }

    //lv_screen_load_anim(app->screen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
    lv_screen_load(app->screen);
}

/*=================== PUBLIC API IMPLEMENTATION ===================*/

int videofocusmanager_init(void)
{
    if (g_vm.initialized) {
        VIDEO_LOGW("Already initialized");
        return APP_OK;
    }

    memset(&g_vm, 0, sizeof(g_vm));
    g_vm.initialized = true;

    VIDEO_LOGI("Video focus manager initialized");
    return APP_OK;
}

void videofocusmanager_deinit(void)
{
    app_reg_t *current;

    if (!g_vm.initialized) {
        return;
    }

    current = get_current_app_reg();
    if (current) {
        notify_focus_change(current->app_id, false);
    }

    for (int i = g_vm.stack_size - 1; i >= 0; i--) {
        app_reg_t *app = find_app_by_id(g_vm.stack[i]);
        if (app) {
            destroy_app_resources(app);
        }
    }

    memset(&g_vm, 0, sizeof(g_vm));
    VIDEO_LOGI("Video focus manager deinitialized");
}

int videofocusmanager_register_app(const app_desc_t *desc)
{
    VIDEO_LOGI("%s", __func__);

    app_reg_t *reg;

    if (!g_vm.initialized) {
        VIDEO_LOGE("Not initialized");
        return APP_ERR_INIT;
    }
    if (!desc || !desc->init_cb) {
        VIDEO_LOGE("Invalid descriptor");
        return APP_ERR_PARAM;
    }

    reg = find_app_by_id(desc->app_id);
    if (reg) {
        VIDEO_LOGW("APP already registered: %s(id=%d)", desc->name, (int)desc->app_id);
        reg->priority = desc->priority;
        reg->name = desc->name;
        reg->init_cb = desc->init_cb;
        reg->destroy_cb = desc->destroy_cb;
        return APP_OK;
    }

    if (g_vm.app_count >= MAX_APPS) {
        VIDEO_LOGE("Max APPs reached (%d)", MAX_APPS);
        return APP_ERR_FULL;
    }

    reg = &g_vm.apps[g_vm.app_count];
    reg->app_id = desc->app_id;
    reg->priority = desc->priority;
    reg->name = desc->name;
    reg->init_cb = desc->init_cb;
    reg->destroy_cb = desc->destroy_cb;
    reg->screen = NULL;
    reg->initialized = false;
    g_vm.app_count++;

    VIDEO_LOGI("Registered APP: %s(id=%d, pri=%s)",
               desc->name, (int)desc->app_id, app_priority_to_str(desc->priority));
    return APP_OK;
}

int videofocusmanager_set_app_priority(app_id_t app_id, app_priority_t priority)
{
    app_reg_t *reg;

    if (!g_vm.initialized) {
        VIDEO_LOGE("Not initialized");
        return APP_ERR_INIT;
    }

    reg = find_app_by_id(app_id);
    if (!reg) {
        VIDEO_LOGE("APP not registered: id=%d", (int)app_id);
        return APP_ERR_PARAM;
    }

    VIDEO_LOGI("APP %s(id=%d) priority: %s -> %s",
               reg->name, (int)app_id,
               app_priority_to_str(reg->priority),
               app_priority_to_str(priority));
    reg->priority = priority;
    return APP_OK;
}

int videofocusmanager_open_app(app_id_t app_id)
{
    app_reg_t *app;
    app_reg_t *current;
    int pos;
    int ret;

    if (!g_vm.initialized) {
        VIDEO_LOGE("Not initialized");
        return APP_ERR_INIT;
    }

    if (app_id == APP_HOME) {
        return videofocusmanager_go_home();
    }

    app = find_app_by_id(app_id);
    if (!app) {
        VIDEO_LOGE("APP not registered: id=%d", (int)app_id);
        return APP_ERR_PARAM;
    }

    current = get_current_app_reg();
    pos = find_in_stack(app_id);
    if (pos == g_vm.stack_size - 1) {
        VIDEO_LOGI("%s(id=%d) already focused", app->name, (int)app_id);
        return APP_OK;
    }

    if (current && current->priority != APP_PRIORITY_SYSTEM &&
        app->priority < current->priority) {
        VIDEO_LOGW("[PRIORITY] Reject: %s(id=%d, pri=%s) < current=%s(id=%d, pri=%s)",
                   app->name, (int)app->app_id, app_priority_to_str(app->priority),
                   current->name, (int)current->app_id, app_priority_to_str(current->priority));
        return APP_ERR_BUSY;
    }

    ret = ensure_app_initialized(app);
    if (ret != APP_OK) {
        return ret;
    }

    if (pos < 0) {
        if (g_vm.stack_size >= MAX_STACK_DEPTH) {
            VIDEO_LOGE("Stack full (max=%d)", MAX_STACK_DEPTH);
            return APP_ERR_FULL;
        }
        push_stack(app_id);
    } else {
        move_stack_entry_to_top(pos);
    }

    if (current) {
        notify_focus_change(current->app_id, false);
    }
    load_app_screen(app);
    notify_focus_change(app_id, true);

    VIDEO_LOGI("Opened %s(id=%d, pri=%s), stack_size=%d",
               app->name, (int)app->app_id,
               app_priority_to_str(app->priority), g_vm.stack_size);
    return APP_OK;
}

int videofocusmanager_close_current_app(void)
{
    app_reg_t *closing_app;
    app_reg_t *restored_app;
    app_id_t closing_id;

    if (!g_vm.initialized) {
        VIDEO_LOGE("Not initialized");
        return APP_ERR_INIT;
    }

    if (g_vm.stack_size <= 1) {
        VIDEO_LOGW("Cannot close: only HOME remains (stack_size=%d)", g_vm.stack_size);
        return APP_ERR_PARAM;
    }

    closing_id = g_vm.stack[g_vm.stack_size - 1];
    closing_app = find_app_by_id(closing_id);
    if (!closing_app) {
        VIDEO_LOGE("Closing APP not found in registry: id=%d", (int)closing_id);
        return APP_ERR_PARAM;
    }

    notify_focus_change(closing_id, false);
    g_vm.stack_size--;
    //for app persist, do not remove screen!
    //destroy_app_resources(closing_app);

    restored_app = get_current_app_reg();
    if (restored_app) {
        load_app_screen(restored_app);
        notify_focus_change(restored_app->app_id, true);
    }

    VIDEO_LOGI("Closed current app: %s(id=%d), stack_size=%d",
               closing_app->name, (int)closing_id, g_vm.stack_size);
    return APP_OK;
}

int videofocusmanager_close_app(app_id_t app_id)
{
    app_reg_t *app;
    app_reg_t *current;
    int pos;
    bool was_top;

    if (!g_vm.initialized) {
        VIDEO_LOGE("Not initialized");
        return APP_ERR_INIT;
    }
    if (app_id == APP_HOME) {
        VIDEO_LOGW("Cannot close HOME");
        return APP_ERR_PARAM;
    }

    pos = find_in_stack(app_id);
    if (pos < 0) {
        VIDEO_LOGW("APP not in stack: id=%d", (int)app_id);
        return APP_ERR_PARAM;
    }

    app = find_app_by_id(app_id);
    if (!app) {
        VIDEO_LOGE("Closing APP not found in registry: id=%d", (int)app_id);
        return APP_ERR_PARAM;
    }

    current = get_current_app_reg();
    was_top = (pos == g_vm.stack_size - 1);
    remove_stack_pos(pos);

    if (was_top && current) {
        notify_focus_change(current->app_id, false);
    }

    //for app persist, do not remove screen!
    //destroy_app_resources(app);

    if (was_top) {
        app_reg_t *new_top = get_current_app_reg();
        if (new_top) {
            load_app_screen(new_top);
            notify_focus_change(new_top->app_id, true);
        }
    }

    VIDEO_LOGI("Closed %s(id=%d), stack_size=%d",
               app->name, (int)app_id, g_vm.stack_size);
    return APP_OK;
}

int videofocusmanager_go_home(void)
{
    app_reg_t *home_app;
    app_reg_t *current;
    int ret;

    if (!g_vm.initialized) {
        VIDEO_LOGE("Not initialized");
        return APP_ERR_INIT;
    }

    home_app = find_app_by_id(APP_HOME);
    if (!home_app) {
        VIDEO_LOGE("HOME not registered");
        return APP_ERR_PARAM;
    }

    current = get_current_app_reg();
    if (g_vm.stack_size == 1 && g_vm.stack[0] == APP_HOME) {
        VIDEO_LOGI("Already on HOME, no-op");
        return APP_OK;
    }

    ret = ensure_app_initialized(home_app);
    if (ret != APP_OK) {
        return ret;
    }

    if (current && current->app_id != APP_HOME) {
        notify_focus_change(current->app_id, false);
    }

    for (int i = g_vm.stack_size - 1; i >= 0; i--) {
        app_id_t id = g_vm.stack[i];
        if (id == APP_HOME) {
            continue;
        }
        //for app persist, do not remove screen!
        //destroy_app_resources(find_app_by_id(id));
    }

    g_vm.stack[0] = APP_HOME;
    g_vm.stack_size = 1;
    load_app_screen(home_app);
    notify_focus_change(APP_HOME, true);

    VIDEO_LOGI("Navigated to HOME, stack reset to [HOME]");
    return APP_OK;
}

app_id_t videofocusmanager_get_current_app(void)
{
    if (!g_vm.initialized || g_vm.stack_size == 0) {
        return APP_NONE;
    }
    return g_vm.stack[g_vm.stack_size - 1];
}

int videofocusmanager_get_stack_size(void)
{
    if (!g_vm.initialized) {
        return -1;
    }
    return g_vm.stack_size;
}

bool videofocusmanager_is_in_stack(app_id_t app_id)
{
    if (!g_vm.initialized) {
        return false;
    }
    return find_in_stack(app_id) >= 0;
}

void videofocusmanager_register_focus_callback(app_id_t app_id, app_focus_change_cb_t cb, void *user_data)
{
    if (!g_vm.initialized) {
        VIDEO_LOGE("Not initialized");
        return;
    }
    if (!cb) {
        VIDEO_LOGE("Null callback");
        return;
    }

    for (int i = 0; i < g_vm.focus_cb_count; i++) {
        if (g_vm.focus_cbs[i].app_id == app_id) {
            VIDEO_LOGD("Updating focus callback for app_id=%d", (int)app_id);
            g_vm.focus_cbs[i].cb = cb;
            g_vm.focus_cbs[i].user_data = user_data;
            return;
        }
    }

    if (g_vm.focus_cb_count >= MAX_FOCUS_CBS) {
        VIDEO_LOGE("Too many focus callbacks registered (max=%d)", MAX_FOCUS_CBS);
        return;
    }

    g_vm.focus_cbs[g_vm.focus_cb_count].app_id = app_id;
    g_vm.focus_cbs[g_vm.focus_cb_count].cb = cb;
    g_vm.focus_cbs[g_vm.focus_cb_count].user_data = user_data;
    g_vm.focus_cb_count++;

    VIDEO_LOGD("Registered focus callback for app_id=%d, total=%d",
               (int)app_id, g_vm.focus_cb_count);
}

void videofocusmanager_unregister_focus_callback(app_id_t app_id)
{
    if (!g_vm.initialized) {
        VIDEO_LOGE("Not initialized");
        return;
    }

    /* Do not call this from a focus callback; notify_focus_change iterates this array. */
    for (int i = 0; i < g_vm.focus_cb_count; i++) {
        if (g_vm.focus_cbs[i].app_id == app_id) {
            VIDEO_LOGD("Unregistering focus callback for app_id=%d", (int)app_id);
            for (int j = i; j < g_vm.focus_cb_count - 1; j++) {
                g_vm.focus_cbs[j] = g_vm.focus_cbs[j + 1];
            }
            g_vm.focus_cb_count--;
            memset(&g_vm.focus_cbs[g_vm.focus_cb_count], 0, sizeof(g_vm.focus_cbs[g_vm.focus_cb_count]));
            return;
        }
    }

    VIDEO_LOGD("Focus callback not registered for app_id=%d", (int)app_id);
}

void videofocusmanager_dump(void)
{
    if (!g_vm.initialized) {
        printf("[VIDEO_DUMP] Not initialized\n");
        return;
    }

    printf("\n========== VIDEO FOCUS DUMP ==========\n");
    printf("Current APP: %s\n", get_current_app_reg() ? get_current_app_reg()->name : "NONE");

    printf("Stack (%d entries, LIFO unique):\n", g_vm.stack_size);
    for (int i = 0; i < g_vm.stack_size; i++) {
        app_reg_t *app = find_app_by_id(g_vm.stack[i]);
        if (app) {
            printf("  [%d] %s(id=%d, pri=%s, screen=%p, init=%d)%s\n",
                   i, app->name, (int)app->app_id,
                   app_priority_to_str(app->priority),
                   (void *)app->screen, (int)app->initialized,
                   (i == g_vm.stack_size - 1) ? " [TOP]" : "");
        } else {
            printf("  [%d] id=%d (NOT REGISTERED)\n", i, (int)g_vm.stack[i]);
        }
    }

    printf("Registered APPs (%d):\n", g_vm.app_count);
    for (int i = 0; i < g_vm.app_count; i++) {
        printf("  %s(id=%d, pri=%s, screen=%p, init=%d)\n",
               g_vm.apps[i].name, (int)g_vm.apps[i].app_id,
               app_priority_to_str(g_vm.apps[i].priority),
               (void *)g_vm.apps[i].screen, (int)g_vm.apps[i].initialized);
    }

    printf("Focus callbacks: %d\n", g_vm.focus_cb_count);
    printf("=======================================\n\n");
}

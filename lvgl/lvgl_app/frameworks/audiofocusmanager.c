/**
 * @file audiofocusmanager.c
 * @brief Audio focus manager with a unique-entry stack model
 *
 * ================== STACK MODEL (LIFO) ==================
 * - Every active request appears at most once in the stack
 * - The current owner is always stack[top]
 * - Transient focus pushes a new owner and keeps lower requests for restore
 * - Permanent focus clears the previous active chain
 */

#include "audiofocusmanager.h"
#include <string.h>

/*=================== CONSTANTS ===================*/
#define MAX_STACK_SIZE  16
#define MAX_ENTRIES     64
#define MAX_CALLBACKS   32

/*=================== INTERNAL TYPES ===================*/
typedef struct {
    bool in_use;
    int id;
    audio_focus_type_t focus_type;
    audio_focus_state_t request_state;
    audio_focus_state_t state;
} focus_entry_t;

typedef struct {
    int id;
    audio_focus_change_callback_t callback;
    void *user_data;
} focus_callback_entry_t;

/*=================== INTERNAL STATE ===================*/
typedef struct {
    focus_entry_t entries[MAX_ENTRIES];
    int stack[MAX_STACK_SIZE];
    uint8_t stack_size;
    focus_callback_entry_t callbacks[MAX_CALLBACKS];
    int callback_count;
    bool initialized;
} audiofocusmanager_state_t;

static audiofocusmanager_state_t g_am = {0};

/*=================== INTERNAL FUNCTIONS ===================*/

static inline bool is_valid_focus_type(audio_focus_type_t type)
{
    return type >= FOCUS_DEFAULT && type < FOCUS_TYPE_MAX;
}

static inline bool is_valid_request_state(audio_focus_state_t state)
{
    return state == FOCUS_GRANTED ||
           state == FOCUS_GRANTED_TRANSIENT ||
           state == FOCUS_GRANTED_TRANSIENT_MAY_DUCK;
}

/**
 * @brief Get priority level for focus type comparison
 *
 * Priority hierarchy: CALL(3) > SPEECH(2) > others(1)
 * All "other" types (DEFAULT, MUSIC, NOTIFICATION, NAVIGATION)
 * share the same priority level, enabling mutual preemption.
 */
static int get_priority_level(audio_focus_type_t type)
{
    switch (type) {
        case FOCUS_CALL:   return 3;
        case FOCUS_SPEECH: return 2;
        default:           return 1;
    }
}

static const char* focus_type_str(audio_focus_type_t type)
{
    switch (type) {
        case FOCUS_CALL:         return "CALL";
        case FOCUS_SPEECH:       return "SPEECH";
        case FOCUS_NAVIGATION:   return "NAVIGATION";
        case FOCUS_NOTIFICATION: return "NOTIFICATION";
        case FOCUS_MUSIC:        return "MUSIC";
        case FOCUS_DEFAULT:      return "DEFAULT";
        default:                 return "UNKNOWN";
    }
}

static const char* state_str(audio_focus_state_t state)
{
    switch (state) {
        case FOCUS_NONE:                      return "NONE";
        case FOCUS_GRANTED:                   return "GRANTED";
        case FOCUS_GRANTED_TRANSIENT:         return "GRANTED_TRANSIENT";
        case FOCUS_GRANTED_TRANSIENT_MAY_DUCK:return "GRANTED_TRANSIENT_MAY_DUCK";
        case FOCUS_LOST:                      return "LOST";
        case FOCUS_LOST_TRANSIENT:            return "LOST_TRANSIENT";
        case FOCUS_LOST_TRANSIENT_MAY_DUCK:   return "LOST_TRANSIENT_MAY_DUCK";
        default:                              return "UNKNOWN";
    }
}

static focus_entry_t* get_entry_by_id(int id)
{
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (g_am.entries[i].in_use && g_am.entries[i].id == id) {
            return &g_am.entries[i];
        }
    }
    return NULL;
}

static focus_entry_t* alloc_entry(void)
{
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (!g_am.entries[i].in_use) {
            memset(&g_am.entries[i], 0, sizeof(g_am.entries[i]));
            g_am.entries[i].in_use = true;
            return &g_am.entries[i];
        }
    }
    return NULL;
}

static void free_entry_by_id(int id)
{
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (g_am.entries[i].in_use && g_am.entries[i].id == id) {
            memset(&g_am.entries[i], 0, sizeof(g_am.entries[i]));
            return;
        }
    }
}

static int find_in_stack_by_id(int id)
{
    for (int i = 0; i < g_am.stack_size; i++) {
        if (g_am.stack[i] == id) {
            return i;
        }
    }
    return -1;
}

static void remove_from_stack_pos(int pos)
{
    if (pos < 0 || pos >= g_am.stack_size) {
        return;
    }

    for (int i = pos; i < g_am.stack_size - 1; i++) {
        g_am.stack[i] = g_am.stack[i + 1];
    }
    g_am.stack_size--;
    g_am.stack[g_am.stack_size] = 0;
}

static void push_to_stack(int id)
{
    g_am.stack[g_am.stack_size] = id;
    g_am.stack_size++;
}

static void notify_focus_change(int id, audio_focus_type_t type, audio_focus_state_t state)
{
    if (!is_valid_focus_type(type)) {
        return;
    }

    for (int i = 0; i < g_am.callback_count; i++) {
        if (g_am.callbacks[i].id == id && g_am.callbacks[i].callback) {
            audio_focus_change_info_t info = {
                .id = id,
                .focus_type = type,
                .state = state
            };
            AUDIO_LOGD("Notifying id=%d type=%s state=%s",
                       id, focus_type_str(type), state_str(state));
            g_am.callbacks[i].callback(&info, g_am.callbacks[i].user_data);
            return;
        }
    }
}

static void set_entry_state(focus_entry_t *entry, audio_focus_state_t new_state)
{
    if (!entry || entry->state == new_state) {
        return;
    }

    entry->state = new_state;
    notify_focus_change(entry->id, entry->focus_type, new_state);
}

static audio_focus_state_t blocked_state_from_parent_request(audio_focus_state_t parent_request_state)
{
    switch (parent_request_state) {
        case FOCUS_GRANTED:
            return FOCUS_LOST;
        case FOCUS_GRANTED_TRANSIENT:
            return FOCUS_LOST_TRANSIENT;
        case FOCUS_GRANTED_TRANSIENT_MAY_DUCK:
            return FOCUS_LOST_TRANSIENT_MAY_DUCK;
        default:
            AUDIO_LOGW("Unexpected parent request state=%d, fallback to LOST_TRANSIENT",
                       (int)parent_request_state);
            return FOCUS_LOST_TRANSIENT;
    }
}

static void refresh_stack_states(void)
{
    for (int i = 0; i < g_am.stack_size; i++) {
        focus_entry_t *entry = get_entry_by_id(g_am.stack[i]);
        audio_focus_state_t new_state;

        if (!entry) {
            continue;
        }

        if (i == g_am.stack_size - 1) {
            new_state = entry->request_state;
            set_entry_state(entry, new_state);
            continue;
        }

        focus_entry_t *parent = get_entry_by_id(g_am.stack[i + 1]);
        new_state = parent ?
            blocked_state_from_parent_request(parent->request_state) :
            FOCUS_LOST_TRANSIENT;
        set_entry_state(entry, new_state);
    }
}

static void notify_and_clear_stack_entries(int skip_id)
{
    while (g_am.stack_size > 0) {
        int id = g_am.stack[g_am.stack_size - 1];
        focus_entry_t *entry = get_entry_by_id(id);

        g_am.stack_size--;
        g_am.stack[g_am.stack_size] = 0;

        if (!entry) {
            continue;
        }
        if (entry->id == skip_id) {
            continue;
        }
        notify_focus_change(entry->id, entry->focus_type, FOCUS_LOST);
        free_entry_by_id(entry->id);
    }
}

/*=================== PUBLIC API ===================*/

int audiofocusmanager_init(void)
{
    if (g_am.initialized) {
        AUDIO_LOGW("Already initialized");
        return AUDIO_OK;
    }

    memset(&g_am, 0, sizeof(g_am));
    g_am.initialized = true;

    AUDIO_LOGI("Audio manager initialized");
    return AUDIO_OK;
}

void audiofocusmanager_deinit(void)
{
    if (!g_am.initialized) {
        return;
    }

    notify_and_clear_stack_entries(-1);

    memset(&g_am, 0, sizeof(g_am));
    AUDIO_LOGI("Audio manager deinitialized");
}

int audiofocusmanager_request_focus(const audio_focus_request_t *request)
{
    focus_entry_t *entry;
    int existing_pos;

    if (!g_am.initialized) {
        AUDIO_LOGE("Not initialized");
        return AUDIO_ERR_INIT;
    }
    if (!request) {
        AUDIO_LOGE("Null request");
        return AUDIO_ERR_PARAM;
    }
    if (!is_valid_focus_type(request->focus_type)) {
        AUDIO_LOGE("Invalid focus type: %d", request->focus_type);
        return AUDIO_ERR_PARAM;
    }
    if (!is_valid_request_state(request->state)) {
        AUDIO_LOGE("Invalid request state: %d", request->state);
        return AUDIO_ERR_PARAM;
    }

    AUDIO_LOGI("Request: type=%s state=%s id=%d",
               focus_type_str(request->focus_type),
               state_str(request->state),
               request->id);

    entry = get_entry_by_id(request->id);
    existing_pos = find_in_stack_by_id(request->id);

    if (entry && existing_pos == g_am.stack_size - 1) {
        audio_focus_type_t old_type = entry->focus_type;
        audio_focus_state_t old_state = entry->state;

        entry->focus_type = request->focus_type;
        entry->request_state = request->state;
        if (request->state == FOCUS_GRANTED && g_am.stack_size > 1) {
            notify_and_clear_stack_entries(request->id);
            push_to_stack(request->id);
        }
        refresh_stack_states();

        /* If the focus type changed but the state stayed the same,
         * refresh_stack_states() will not notify. Notify explicitly so
         * that the listener knows it is now handling a different stream
         * type (e.g. CarPlay switching from alert to telephony). */
        if (old_state == entry->state && old_type != entry->focus_type) {
            notify_focus_change(entry->id, entry->focus_type, entry->state);
        }
        return AUDIO_OK;
    }

    if (g_am.stack_size > 0) {
        focus_entry_t *owner = get_entry_by_id(g_am.stack[g_am.stack_size - 1]);
        if (owner) {
            int req_prio = get_priority_level(request->focus_type);
            int owner_prio = get_priority_level(owner->focus_type);
            if (req_prio < owner_prio) {
                AUDIO_LOGW("[PRIORITY] Reject: %s(%d) < current=%s(%d)",
                           focus_type_str(request->focus_type), req_prio,
                           focus_type_str(owner->focus_type), owner_prio);

                return AUDIO_ERR_BUSY;
            }
        }
    }

    {
        audio_focus_type_t old_type = entry ? entry->focus_type : FOCUS_DEFAULT;
        audio_focus_state_t old_state = entry ? entry->state : FOCUS_NONE;

        if (existing_pos >= 0) {
            remove_from_stack_pos(existing_pos);
        } else if (entry) {
            free_entry_by_id(request->id);
            entry = NULL;
        }

        if (!entry) {
            entry = alloc_entry();
            if (!entry) {
                AUDIO_LOGE("No free entry slot for id=%d", request->id);
                return AUDIO_ERR_FULL;
            }
            entry->id = request->id;
        }

        entry->focus_type = request->focus_type;
        entry->request_state = request->state;

        if (request->state == FOCUS_GRANTED) {
            notify_and_clear_stack_entries(request->id);
        } else if (g_am.stack_size >= MAX_STACK_SIZE) {
            AUDIO_LOGE("Stack full (max=%d)", MAX_STACK_SIZE);
            free_entry_by_id(request->id);
            return AUDIO_ERR_FULL;
        }

        push_to_stack(request->id);
        refresh_stack_states();

        /* See the top-of-stack branch above for the rationale. */
        if (old_state == entry->state && old_type != entry->focus_type) {
            notify_focus_change(entry->id, entry->focus_type, entry->state);
        }
    }

    AUDIO_LOGI("Granted focus to id=%d type=%s state=%s, stack_size=%d",
               request->id, focus_type_str(request->focus_type),
               state_str(request->state), g_am.stack_size);
    return AUDIO_OK;
}

void audiofocusmanager_release_focus(int id)
{
    int pos;

    if (!g_am.initialized) {
        AUDIO_LOGE("Not initialized");
        return;
    }

    pos = find_in_stack_by_id(id);
    if (pos < 0) {
        AUDIO_LOGW("No active focus found with id=%d", id);
        free_entry_by_id(id);
        return;
    }

    AUDIO_LOGI("Releasing focus id=%d", id);

    remove_from_stack_pos(pos);
    free_entry_by_id(id);
    refresh_stack_states();

    AUDIO_LOGI("Focus id=%d released, owner now=%s",
               id,
               g_am.stack_size > 0 ?
               focus_type_str(get_entry_by_id(g_am.stack[g_am.stack_size - 1])->focus_type) :
               "NONE");
}

bool audiofocusmanager_get_owner_info(focus_owner_info_t *info)
{
    focus_entry_t *owner;

    if (!g_am.initialized || !info || g_am.stack_size == 0) {
        return false;
    }

    owner = get_entry_by_id(g_am.stack[g_am.stack_size - 1]);
    if (!owner) {
        return false;
    }

    info->id = owner->id;
    info->focus_type = owner->focus_type;
    info->state = owner->state;
    return true;
}

void audiofocusmanager_register_focus_callback(int id,
                                          audio_focus_change_callback_t callback,
                                          void *user_data)
{
    if (!g_am.initialized) {
        AUDIO_LOGE("Not initialized");
        return;
    }
    if (!callback) {
        AUDIO_LOGE("Null callback");
        return;
    }

    for (int i = 0; i < g_am.callback_count; i++) {
        if (g_am.callbacks[i].id == id) {
            AUDIO_LOGD("Updating callback for id=%d", id);
            g_am.callbacks[i].callback = callback;
            g_am.callbacks[i].user_data = user_data;
            return;
        }
    }

    if (g_am.callback_count >= MAX_CALLBACKS) {
        AUDIO_LOGE("Too many callbacks registered");
        return;
    }

    AUDIO_LOGD("Registering focus callback for id=%d", id);
    g_am.callbacks[g_am.callback_count].id = id;
    g_am.callbacks[g_am.callback_count].callback = callback;
    g_am.callbacks[g_am.callback_count].user_data = user_data;
    g_am.callback_count++;
}

void audiofocusmanager_unregister_focus_callback(int id)
{
    if (!g_am.initialized) {
        AUDIO_LOGE("Not initialized");
        return;
    }

    for (int i = 0; i < g_am.callback_count; i++) {
        if (g_am.callbacks[i].id == id) {
            AUDIO_LOGD("Unregistering focus callback for id=%d", id);
            for (int j = i; j < g_am.callback_count - 1; j++) {
                g_am.callbacks[j] = g_am.callbacks[j + 1];
            }
            g_am.callback_count--;
            return;
        }
    }
}

void audiofocusmanager_dump(void)
{
    if (!g_am.initialized) {
        printf("[AUDIO_DUMP] Not initialized\n");
        return;
    }

    printf("\n========== AUDIO FOCUS DUMP ==========\n");
    if (g_am.stack_size > 0) {
        focus_entry_t *owner = get_entry_by_id(g_am.stack[g_am.stack_size - 1]);
        printf("Owner: id=%d type=%s state=%s\n",
               owner ? owner->id : -1,
               owner ? focus_type_str(owner->focus_type) : "NONE",
               owner ? state_str(owner->state) : "NONE");
    } else {
        printf("Owner: NONE\n");
    }

    printf("Stack (%d entries, LIFO unique):\n", g_am.stack_size);
    for (int i = 0; i < g_am.stack_size; i++) {
        focus_entry_t *entry = get_entry_by_id(g_am.stack[i]);
        if (!entry) {
            continue;
        }

        printf("  [%d] id=%d type=%s request=%s state=%s%s\n",
               i,
               entry->id,
               focus_type_str(entry->focus_type),
               state_str(entry->request_state),
               state_str(entry->state),
               (i == g_am.stack_size - 1) ? " [TOP]" : "");
    }
    printf("========================================\n\n");
}

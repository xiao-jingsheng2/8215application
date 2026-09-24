// src/MVVM/settings_vm.c
#include "settings_vm.h"
#include "awtk.h"
#include "MVVM/model/settings_model.h"

static bool_t            g_is_init = FALSE;

static mvvm_emitter_t    g_emitter;
static settings_change_t g_pending;       // emit 同步期间复用的 payload（与 dashboard_vm 同构）
static settings_model_t  g_last_seen;     // diff cache：避免 model emit 全量字段时重复通知

static void emit_field(settings_field_e f, const void* v) {
    g_pending.field = f;
    g_pending.value = v;
    mvvm_emitter_emit(&g_emitter, &g_pending);
}

static void on_settings_model_changed(void* ctx, const void* value) {
    (void)ctx;
    if (value == NULL) return;
    const settings_model_t* cur = (const settings_model_t*)value;

    if (cur->lang != g_last_seen.lang) {
        g_last_seen.lang = cur->lang;
        emit_field(SETTINGS_FIELD_LANG, &cur->lang);
    }
    if (cur->unit != g_last_seen.unit) {
        g_last_seen.unit = cur->unit;
        emit_field(SETTINGS_FIELD_UNIT, &cur->unit);
    }
    if (cur->brightness != g_last_seen.brightness) {
        g_last_seen.brightness = cur->brightness;
        emit_field(SETTINGS_FIELD_BRIGHTNESS, &cur->brightness);
    }
    if (cur->display != g_last_seen.display) {
        g_last_seen.display = cur->display;
        emit_field(SETTINGS_FIELD_DISPLAY, &cur->display);
    }
    if (cur->bluetooth != g_last_seen.bluetooth) {
        g_last_seen.bluetooth = cur->bluetooth;
        emit_field(SETTINGS_FIELD_BLUETOOTH, &cur->bluetooth);
    }
}

ret_t settings_vm_init(void) {
    if (g_is_init) return RET_OK;

    mvvm_emitter_init(&g_emitter, "settings.vm");

    settings_model_init();
    // 同步初始快照，避免 init 后 model 第一次 emit 触发一遍 VM 全字段通知。
    g_last_seen = *settings_model_get_data();

    mvvm_emitter_on(settings_model_get_emitter(),
                    on_settings_model_changed, NULL);

    g_is_init = TRUE;
    return RET_OK;
}

mvvm_emitter_t* settings_vm_get_emitter(void) {
    return &g_emitter;
}

ret_t settings_vm_set(settings_field_e field, const void* value) {
    return_value_if_fail(field < SETTINGS_FIELD_MAX, RET_BAD_PARAMS);
    return_value_if_fail(value != NULL, RET_BAD_PARAMS);
    return settings_model_set(field, value);
}

settings_lang_e    settings_vm_get_language(void)   { return settings_model_get_data()->lang; }
settings_unit_e    settings_vm_get_unit(void)       { return settings_model_get_data()->unit; }
uint8_t            settings_vm_get_brightness(void) { return settings_model_get_data()->brightness; }
settings_display_e settings_vm_get_display(void)    { return settings_model_get_data()->display; }
bool_t             settings_vm_get_bluetooth(void)  { return settings_model_get_data()->bluetooth; }

const char* settings_vm_format_speed(int32_t kmh, settings_unit_e unit) {
    static char buf[32];
    if (unit == SETTINGS_UNIT_MILE) {
        tk_snprintf(buf, sizeof(buf), "%d mph", (int)(kmh * 0.621371f));
    } else {
        tk_snprintf(buf, sizeof(buf), "%d km/h", (int)kmh);
    }
    return buf;
}

const char* settings_vm_format_distance(int32_t km, settings_unit_e unit) {
    static char buf[32];
    if (unit == SETTINGS_UNIT_MILE) {
        tk_snprintf(buf, sizeof(buf), "%d mile", (int)(km * 0.621371f));
    } else {
        tk_snprintf(buf, sizeof(buf), "%d km", (int)km);
    }
    return buf;
}

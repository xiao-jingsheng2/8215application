// src/model/settings_model.c
#include "MVVM/model/settings_model.h"
#include <string.h>
#include "proxy/vehicle_argument.h"

// static 变量不能用函数调用初始化（C 不允许 static-storage initializer 含非常量表达式），
// 所有初值搬到 init() 里从 proxy 读取。
static settings_model_t g_state;
static mvvm_emitter_t   g_em;
static bool             g_is_init = false;

int settings_model_init(void) {
    if (g_is_init) return 0;
    g_state.lang       = (settings_lang_e)vehicle_get_param_language();
    g_state.unit       = (settings_unit_e)vehicle_get_param_unit();
    g_state.brightness = vehicle_get_param_brightness();
    g_state.display    = (settings_display_e)vehicle_get_param_display();
    g_state.bluetooth  = vehicle_get_param_bluetooth();
    mvvm_emitter_init(&g_em, "settings.model");

    g_is_init = true;
    return 0;
}

mvvm_emitter_t* settings_model_get_emitter(void) { return &g_em; }

const settings_model_t* settings_model_get_data(void) { return &g_state; }

static bool field_changed(settings_field_e f, const void* v) {
    switch (f) {
    case SETTINGS_FIELD_LANG:       return *(settings_lang_e*)v    != g_state.lang;
    case SETTINGS_FIELD_UNIT:       return *(settings_unit_e*)v    != g_state.unit;
    case SETTINGS_FIELD_BRIGHTNESS: return *(uint8_t*)v            != g_state.brightness;
    case SETTINGS_FIELD_DISPLAY:    return *(settings_display_e*)v != g_state.display;
    case SETTINGS_FIELD_BLUETOOTH:  return *(bool*)v               != g_state.bluetooth;
    default:                        return false;
    }
}

static void apply_field(settings_field_e f, const void* v) {
    switch (f) {
    case SETTINGS_FIELD_LANG:       g_state.lang       = *(settings_lang_e*)v;    break;
    case SETTINGS_FIELD_UNIT:       g_state.unit       = *(settings_unit_e*)v;    break;
    case SETTINGS_FIELD_BRIGHTNESS: g_state.brightness = *(uint8_t*)v;            break;
    case SETTINGS_FIELD_DISPLAY:    g_state.display    = *(settings_display_e*)v; break;
    case SETTINGS_FIELD_BLUETOOTH:  g_state.bluetooth  = *(bool*)v;               break;
    default:                        break;
    }
}

int settings_model_set(settings_field_e field, const void* value) {
    if (value == NULL) return -1;
    if (field >= SETTINGS_FIELD_MAX) return -1;
    if (!field_changed(field, value)) return 0;
    apply_field(field, value);
    mvvm_emitter_emit(&g_em, &g_state);
    return 0;
}

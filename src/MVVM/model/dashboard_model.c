// src/model/dashboard_model.c
#include "dashboard_model.h"
#include <string.h>

static bool             g_t_init = false;
static dashboard_model_t  g_state;
static mvvm_emitter_t     g_em;

static bool state_changed(const dashboard_model_t* a,
                          const dashboard_model_t* b) {
    if (a->speed != b->speed)     return true;
    if (a->rpm != b->rpm)         return true;
    if (a->gear != b->gear)       return true;
    if (a->power != b->power)     return true;
    if (a->battery != b->battery) return true;
    if (memcmp(a->signals, b->signals, sizeof(a->signals)) != 0) return true;
    return false;
}

int dashboard_model_init(void) {
    if (g_t_init) return 0;

    mvvm_emitter_init(&g_em, "dashboard.model");

    memset(&g_state, 0, sizeof(g_state));
    g_state.gear = DASHBOARD_GEAR_INVALID;
    g_t_init = true;

    return 0;
}

void dashboard_model_deinit(void) {
    if (!g_t_init) return;
    mvvm_emitter_deinit(&g_em);
    g_t_init = false;
}

mvvm_emitter_t* dashboard_model_get_emitter(void) {
    return &g_em;
}

const dashboard_model_t* dashboard_model_get_data(void) {
    return &g_state;
}

static bool field_changed(dashboard_field_e f, const void* v) {
    switch (f) {
    case DASHBOARD_FIELD_SPEED:   return *(int32_t*)v          != g_state.speed;
    case DASHBOARD_FIELD_RPM:     return *(int32_t*)v          != g_state.rpm;
    case DASHBOARD_FIELD_GEAR:    return *(dashboard_gear_e*)v != g_state.gear;
    case DASHBOARD_FIELD_POWER:   return *(int32_t*)v          != g_state.power;
    case DASHBOARD_FIELD_BATTERY: return *(int32_t*)v          != g_state.battery;
    case DASHBOARD_FIELD_SIGNALS: return memcmp(v, g_state.signals,
                                                sizeof(g_state.signals)) != 0;
    default:                      return false;
    }
}

static void apply_field(dashboard_field_e f, const void* v) {
    switch (f) {
    case DASHBOARD_FIELD_SPEED:   g_state.speed   = *(int32_t*)v;          break;
    case DASHBOARD_FIELD_RPM:     g_state.rpm     = *(int32_t*)v;          break;
    case DASHBOARD_FIELD_GEAR:    g_state.gear    = *(dashboard_gear_e*)v; break;
    case DASHBOARD_FIELD_POWER:   g_state.power   = *(int32_t*)v;          break;
    case DASHBOARD_FIELD_BATTERY: g_state.battery = *(int32_t*)v;          break;
    case DASHBOARD_FIELD_SIGNALS: memcpy(g_state.signals, v,
                                         sizeof(g_state.signals));        break;
    default:                      break;
    }
}

int dashboard_model_set(dashboard_field_e field, const void* value) {
    if (value == NULL) return -1;
    if (field >= DASHBOARD_FIELD_MAX) return -1;
    if (!field_changed(field, value)) return 0;
    apply_field(field, value);
    mvvm_emitter_emit(&g_em, &g_state);
    return 0;
}

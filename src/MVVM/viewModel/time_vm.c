// src/MVVM/time_vm.c
#include "time_vm.h"
#include "awtk.h"
#include "proxy/vehicle_time.h"

static bool_t g_is_init = FALSE;
static mvvm_emitter_t g_time_em;
static time_data_t last_time = {0, 0, FALSE};
static uint32_t g_timer_id = 0;

static ret_t on_time_timer(const timer_info_t* info) {
    (void)info;
    time_vm_poll_once();
    return RET_REPEAT;
}

void time_vm_poll_once(void) {
    if (!g_is_init) return;

    int32_t h = vehicle_get_time_hour();
    int32_t m = vehicle_get_time_min();
    bool_t colon = !last_time.colon_visible;

    if (h != last_time.hour || m != last_time.minute || colon != last_time.colon_visible) {
        last_time.hour = h;
        last_time.minute = m;
        last_time.colon_visible = colon;
        mvvm_emitter_emit(&g_time_em, &last_time);
    }
}

ret_t time_vm_init(void) {
    if (g_is_init) return RET_OK;
    mvvm_emitter_init(&g_time_em, "time");
    last_time.hour = 0;
    last_time.minute = 0;
    last_time.colon_visible = FALSE;
    g_timer_id = timer_add(on_time_timer, NULL, 500);
    g_is_init = TRUE;
    time_vm_poll_once();
    return RET_OK;
}

mvvm_emitter_t* time_vm_get_time_emitter(void) { return &g_time_em; }

ret_t time_vm_set_time(int32_t hour, int32_t minute) {
    if (!g_is_init) return RET_OK;
    if (hour < 0 || hour > 23) return RET_FAIL;
    if (minute < 0 || minute > 59) return RET_FAIL;
    vehicle_set_time(hour, minute);
    time_vm_poll_once();
    return RET_OK;
}

void time_vm_get_current(int32_t* hour, int32_t* minute) {
    if (hour) *hour = last_time.hour;
    if (minute) *minute = last_time.minute;
}